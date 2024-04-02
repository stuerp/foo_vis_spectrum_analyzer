
/** $VER: Spectogram.cpp (2024.04.02) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "Spectogram.h"

#include "Support.h"

#include "DirectWrite.h"

#pragma hdrstop

Spectogram::Spectogram()
{
    _Bounds = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void Spectogram::Initialize(State * state, const GraphSettings * settings, const FrequencyBands & frequencyBands)
{
    _State = state;
    _GraphSettings = settings;

    ReleaseDeviceSpecificResources();

    InitYAxis(frequencyBands);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Spectogram::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;
    _Size = { rect.right - rect.left, rect.bottom - rect.top };

    _Bitmap.Release();
    _BitmapRenderTarget.Release();

    _IsResized = true;
}

/// <summary>
/// Resets this instance.
/// </summary>
void Spectogram::Reset()
{
    _X = 0.f;
    _PlaybackTime = 0.;
    _TrackTime = 0.;
    _RequestErase = true;

    _XLabels.clear();

    _Bitmap.Release();
    _BitmapRenderTarget.Release();

    _IsResized = true;
}

/// <summary>
/// Renders the spectrum analysis as a spectogram.
/// </summary>
void Spectogram::Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    // Update the offscreen bitmap.
    if (_State->_PlaybackTime != _PlaybackTime) // Not paused
        Update(frequencyBands, _State->_TrackTime, sampleRate / 2.f);

    // Draw the offscreen bitmap.
    {
        SetTransform(renderTarget, _BitmapBounds);

        if (_State->_ScrollingSpectogram)
        {
            D2D1_RECT_F Src = D2D1_RECT_F(_X + 1.f, 0.f, _BitmapSize.width,      _BitmapSize.height);
            D2D1_RECT_F Dst = D2D1_RECT_F(     0.f, 0.f, _BitmapSize.width - _X, _BitmapSize.height);

            renderTarget->DrawBitmap(_Bitmap, &Dst, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);

            Src.right = Src.left;
            Src.left  = 0.f;

            Dst.left  = Dst.right;
            Dst.right = _Size.width;

            renderTarget->DrawBitmap(_Bitmap, &Dst, _SpectogramStyle->_Opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);
        }
        else
        {
            D2D1_RECT_F Rect = D2D1_RECT_F(0.f, 0.f, _BitmapSize.width, _BitmapSize.height);

            renderTarget->DrawBitmap(_Bitmap, &Rect, _SpectogramStyle->_Opacity);
        }

        ResetTransform(renderTarget);
    }

    // Draw the X-axis (Time).
    if (!_XLabels.empty())
    {
        if (_GraphSettings->_XAxisTop)
            RenderXAxis(renderTarget, true);

        if (_GraphSettings->_XAxisBottom)
            RenderXAxis(renderTarget, false);
    }

    // Draw the Y-axis (Frequency).
    if (!_YLabels.empty())
    {
        if (_GraphSettings->_YAxisLeft)
            RenderYAxis(renderTarget, true);

        if (_GraphSettings->_YAxisRight)
            RenderYAxis(renderTarget, false);
    }

    if (_State->_PlaybackTime != _PlaybackTime) // Not paused
    {
        _X++;

        if (_X > _Size.width)
        {
            _X = 0.f;

            if (!_State->_ScrollingSpectogram)
                _XLabels.clear();
        }
    }

    _PlaybackTime = _State->_PlaybackTime;
    _TrackTime = _State->_TrackTime;
}

/// <summary>
/// Renders an X-axis (Time)
/// </summary>
void Spectogram::RenderXAxis(ID2D1RenderTarget * renderTarget, bool top) const noexcept
{
    const FLOAT y1 = top ? _XTextStyle->_TextHeight / 2.f : _Size.height - _XTextStyle->_TextHeight;
    const FLOAT y2 = top ? _XTextStyle->_TextHeight       : y1 + (_XTextStyle->_TextHeight / 2.f);

    D2D1_RECT_F Rect = { 0.f, top ? 0.f : y1, 0.f, top ? y2 : _Size.height };

    for (const auto & Label : _XLabels)
    {
        // Draw the tick.
        renderTarget->DrawLine( { Label.X, y1 }, { Label.X, y2 }, _XLineStyle->_Brush, _XLineStyle->_Thickness);

        if (!_GraphSettings->_FlipHorizontally)
        {
            Rect.left  = Label.X + Offset;
            Rect.right = Rect.left + _XTextStyle->_TextWidth;

            renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        else
        {
            Rect.left  = Label.X - Offset;
            Rect.right = Rect.left - _XTextStyle->_TextWidth;

            renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }
}

/// <summary>
/// Renders a Y-axis (Frequency)
/// </summary>
void Spectogram::RenderYAxis(ID2D1RenderTarget * renderTarget, bool left) const noexcept
{
    const double MinScale = ScaleF(_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

    D2D1_RECT_F Rect =
    {
        left ? 0.f : _Size.width - _YTextStyle->_TextWidth,
        0.f,
        left ? _YTextStyle->_TextWidth : _Size.width,
        0.f
    };

    const FLOAT dy = _YTextStyle->_TextHeight / 2.f;

    const FLOAT y1 = _GraphSettings->_XAxisTop ? _XTextStyle->_TextHeight : 0.f;

    for (const auto & Label : _YLabels)
    {
        const FLOAT y = Map(ScaleF(Label.Frequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale, 0.f, _BitmapSize.height);

        if (!_GraphSettings->_FlipVertically)
        {
            Rect.top    = y1 + _BitmapSize.height - y - dy;
            Rect.bottom = Rect.top + _YTextStyle->_TextHeight;

            if (Rect.bottom < (y1 - dy))
                break;
        }
        else
        {
            Rect.top    = y1 + y - dy;
            Rect.bottom = Rect.top + _YTextStyle->_TextHeight;

            if (Rect.bottom > (y1 + _BitmapSize.height + dy))
                break;
        }

        renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _YTextStyle->_TextFormat, Rect, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
}

/// <summary>
/// Updates this instance.
/// </summary>
void Spectogram::Update(const FrequencyBands & frequencyBands, double trackTime, double nyquistFrequency) noexcept
{
    _BitmapRenderTarget->BeginDraw();

    if (_RequestErase)
    {
        _BitmapRenderTarget->Clear(); // Make the bitmap completely transparent.

        _RequestErase = false;
    }

    // Draw the next spectogram line.
    {
        const FLOAT Bandwidth = Max((_BitmapSize.height / (FLOAT) _BandCount), 1.f);

        FLOAT y1 = 0.f;
        FLOAT y2 = Bandwidth;

        for (const auto & fb : frequencyBands)
        {
            if ((fb.Ctr >= nyquistFrequency) && _State->_SuppressMirrorImage)
                break;

            assert(InRange(fb.CurValue, 0.0, 1.0));

            _SpectogramStyle->SetBrushColor(fb.CurValue);

            _BitmapRenderTarget->DrawLine({ _X, y1 }, { _X, y2 }, _SpectogramStyle->_Brush);

            y1  = y2;
            y2 += Bandwidth;
        }
    }

    // Draw the Nyquist marker.
    if (_NyquistMarker->_ColorSource != ColorSource::None)
        RenderNyquistFrequencyMarker(_BitmapRenderTarget, frequencyBands, nyquistFrequency);

    _BitmapRenderTarget->EndDraw();

    // Update the X-axis.
    if (_State->_ScrollingSpectogram && (_State->_PlaybackTime != _PlaybackTime))
    {
        for (auto & Label : _XLabels)
            if (!_GraphSettings->_FlipHorizontally)
                Label.X--; // Scroll to the left.
            else
                Label.X++; // Scroll to the right.
    }

    if (_TrackTime != trackTime) // in seconds
    {
        if (_State->_ScrollingSpectogram)
        {
            _XLabels.push_front({ pfc::wideFromUTF8(pfc::format_time((uint64_t) trackTime)), _GraphSettings->_FlipHorizontally ? 0.f : _Size.width });

            if (_XLabels.back().X + Offset + _XTextStyle->_TextWidth < 0.f)
                _XLabels.pop_back();
        }
        else
            _XLabels.push_back({ pfc::wideFromUTF8(pfc::format_time((uint64_t) trackTime)), _GraphSettings->_FlipHorizontally ? _Size.width - _X : _X });
    }
}

/// <summary>
/// Renders a marker for the Nyquist frequency.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void Spectogram::RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double nyquistFrequency) const noexcept
{
    const double MinScale = ScaleF(frequencyBands.front().Ctr, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(frequencyBands.back() .Ctr, _State->_ScalingFunction, _State->_SkewFactor);

    const double NyquistScale = Clamp(ScaleF(nyquistFrequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale);

    FLOAT y = Map(NyquistScale, MinScale, MaxScale, 0.f, _BitmapSize.height);

    renderTarget->DrawLine(D2D1_POINT_2F(_X, y), D2D1_POINT_2F(_X + 1, y), _NyquistMarker->_Brush, _NyquistMarker->_Thickness, nullptr);
}

/// <summary>
/// Initializes the Y-axis.
/// </summary>
void Spectogram::InitYAxis(const FrequencyBands & frequencyBands) noexcept
{
    _YLabels.clear();

    if (frequencyBands.size() == 0)
        return;

    _BandCount = frequencyBands.size();

    _LoFrequency = frequencyBands.front().Ctr;
    _HiFrequency = frequencyBands.back().Ctr;

    // Precalculate the labels.
    {
        WCHAR Text[32] = { };

        switch (_GraphSettings->_XAxisMode)
        {
            case XAxisMode::None:
                break;

            default:

            case XAxisMode::Bands:
            {
                for (size_t i = 0; i < frequencyBands.size(); i += 10)
                {
                    double Frequency = frequencyBands[i].Ctr;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.f", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fk", Frequency / 1000.);

                    YLabel lb = { Text, Frequency };

                    _YLabels.push_back(lb);
                }
                break;
            }

            case XAxisMode::Decades:
            {
                double Frequency = 0.;
                int i = 1;
                int j = 10;

                while (Frequency < frequencyBands.back().Lo)
                {
                    Frequency = j * i;

                    if (Frequency < 1000.)
                        ::StringCchPrintfW(Text, _countof(Text), L"%.f", Frequency);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%.1fk", Frequency / 1000.);

                    YLabel lb = { Text, Frequency };

                    _YLabels.push_back(lb);

                    if (++i == 10)
                    {
                        i = 1;
                        j *= 10;
                    }
                }
                break;
            }

            case XAxisMode::Octaves:
            {
                double Note = -57.;                                     // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _State->_Pitch * ::exp2(Note / 12.); // Frequency of C0

                for (int i = 0; Frequency < frequencyBands.back().Lo; ++i)
                {
                    ::StringCchPrintfW(Text, _countof(Text), L"C%d", i);

                    YLabel lb = { Text, Frequency };

                    _YLabels.push_back(lb);

                    Note += 12.;
                    Frequency = _State->_Pitch * ::exp2(Note / 12.);
                }
                break;
            }

            case XAxisMode::Notes:
            {
                static const char Name[] = { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
                static const int Step[] = { 2, 2, 1, 2, 2, 2, 1 };

                double Note = -57.;                                     // Index of C0 (57 semi-tones lower than A4 at 440Hz)
                double Frequency = _State->_Pitch * ::exp2(Note / 12.); // Frequency of C0

                int j = 0;

                while (Frequency < frequencyBands.back().Lo)
                {
                    int Octave = (int) ((Note + 57.) / 12.);

                    if (j == 0)
                        ::StringCchPrintfW(Text, _countof(Text), L"%c%d", Name[j], Octave);
                    else
                        ::StringCchPrintfW(Text, _countof(Text), L"%c", Name[j]);

                    YLabel lb = { Text, Frequency, j != 0 };

                    _YLabels.push_back(lb);

                    Note += Step[j];
                    Frequency = _State->_Pitch * ::exp2(Note / 12.);

                    if (j < 6) j++; else j = 0;
                }
                break;
            }
        }
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT Spectogram::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Spectogram, renderTarget, _Size, L"", &_SpectogramStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, renderTarget, _Size, L"", &_XLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, _Size, L"00:00", &_XTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, _Size, L"", &_YLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, _Size, L"99.9fk", &_YTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::NyquistMarker, renderTarget, _Size, L"", &_NyquistMarker);

    if (SUCCEEDED(hr))
        Resize();

    // Create the offscreen bitmap resources.
    {
        if (SUCCEEDED(hr) && (_BitmapRenderTarget == nullptr))
            hr = renderTarget->CreateCompatibleRenderTarget(_BitmapSize, &_BitmapRenderTarget);

        if (SUCCEEDED(hr) && (_Bitmap == nullptr))
            hr = _BitmapRenderTarget->GetBitmap(&_Bitmap);
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Spectogram::ReleaseDeviceSpecificResources()
{
    _Bitmap.Release();
    _BitmapRenderTarget.Release();

    if (_NyquistMarker)
    {
        _NyquistMarker->ReleaseDeviceSpecificResources();
        _NyquistMarker = nullptr;
    }

    if (_YTextStyle)
    {
        _YTextStyle->ReleaseDeviceSpecificResources();
        _YTextStyle = nullptr;
    }

    if (_YLineStyle)
    {
        _YLineStyle->ReleaseDeviceSpecificResources();
        _YLineStyle = nullptr;
    }

    if (_XTextStyle)
    {
        _XTextStyle->ReleaseDeviceSpecificResources();
        _XTextStyle = nullptr;
    }

    if (_XLineStyle)
    {
        _XLineStyle->ReleaseDeviceSpecificResources();
        _XLineStyle = nullptr;
    }

    if (_SpectogramStyle)
    {
        _SpectogramStyle->ReleaseDeviceSpecificResources();
        _SpectogramStyle = nullptr;
    }
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void Spectogram::Resize() noexcept
{
    if (!_IsResized)
        return;

    _XTextStyle->SetHorizontalAlignment(_GraphSettings->_FlipHorizontally ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);
    _YTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

    _BitmapBounds = _Bounds;

    if (_GraphSettings->_XAxisTop)
        _BitmapBounds.top += _XTextStyle->_TextHeight;

    if (_GraphSettings->_XAxisBottom)
        _BitmapBounds.bottom -= _XTextStyle->_TextHeight;

    _BitmapSize = { _BitmapBounds.right - _BitmapBounds.left, _BitmapBounds.bottom - _BitmapBounds.top };

    _IsResized = false;
}

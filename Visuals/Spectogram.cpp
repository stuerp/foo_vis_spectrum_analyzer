
/** $VER: Spectogram.cpp (2024.03.22) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "Spectogram.h"

#include "Support.h"

#include "DirectWrite.h"

#pragma hdrstop

Spectogram::Spectogram()
{
    _Bounds = { };
    _Size = { };

    _FontFamilyName = L"Segoe UI";
    _FontSize = 6.f;

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

}

/// <summary>
/// Resets this instance.
/// </summary>
void Spectogram::Reset()
{
    _X = 0;
    _PlaybackTime = 0.;
    _TrackTime = 0.;
    _RequestErase = true;

    _XLabels.clear();

    _Bitmap.Release();
    _BitmapRenderTarget.Release();
}

/// <summary>
/// Renders the spectrum analysis as a spectogram.
/// </summary>
void Spectogram::Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    // Update the spectogram bitmap.
    if (_State->_PlaybackTime != _PlaybackTime)
        Update(frequencyBands, _State->_TrackTime, sampleRate);

    // Determine and set the transform.
    {
        D2D1::Matrix3x2F Transform = D2D1::Matrix3x2F::Identity();

        if (_GraphSettings->_FlipHorizontally)
            Transform = D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, _BitmapSize.width, 0.f);

        if (!_GraphSettings->_FlipVertically) // Negate because the GUI assumes the mathematical (bottom-left 0,0) coordinate system.
        {
            const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, _BitmapSize.height);

            Transform = Transform * FlipV;
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_Bounds.left, _Bounds.top);

        renderTarget->SetTransform(Transform * Translate);
    }

    // Draw the bitmap.
    {
        if (_State->_ScrollingSpectogram)
        {
            D2D1_RECT_F Src = D2D1_RECT_F((FLOAT) (_X + 1), 0.f, _BitmapSize.width,              _BitmapSize.height);
            D2D1_RECT_F Dst = D2D1_RECT_F(             0.f, 0.f, _BitmapSize.width - (FLOAT) _X, _BitmapSize.height);

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
    }

    // Reset the transform.
    renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    // Draw the X-axis (Time).
    if (!_XLabels.empty())
    {
        if (_GraphSettings->_XAxisTop)
            RenderXAxis(renderTarget, true);

        if (_GraphSettings->_XAxisBottom)
            RenderXAxis(renderTarget, false);

        if (_State->_ScrollingSpectogram)
        {
            if (_XLabels.back().X + Offset + _XTextWidth < 0.f)
                _XLabels.pop_back();
        }
        else
        {
            if ((FLOAT) _X + Offset + _XTextWidth > _Size.width)
                _XLabels.pop_back();
        }
    }

    // Draw the Y-axis (Frequency).
    if (!_YLabels.empty())
    {
        const double MinScale = ScaleF(_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
        const double MaxScale = ScaleF(_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

        D2D1_RECT_F Rect = { 0.f, 0.f, _YTextWidth, 0.f };

        for (auto & Label : _YLabels)
        {
            const FLOAT y = Map(ScaleF(Label.Frequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale, 0.f, _BitmapSize.height);

            if (!_GraphSettings->_FlipVertically)
            {
                Rect.top    = _BitmapSize.height - (y + _YTextHeight);
                Rect.bottom = Rect.top +_YTextHeight;
            }
            else
            {
                Rect.top    = y;
                Rect.bottom = Rect.top +_YTextHeight;
            }

            if ((Rect.top < 0.f) || (Rect.bottom > _BitmapSize.height))
                break;

            renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _YTextFormat, Rect, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }

    if (_State->_PlaybackTime != _PlaybackTime)
    {
        _X++;

        if (_X > (uint32_t) _Size.width)
            _X = 0;
    }

    _PlaybackTime = _State->_PlaybackTime;
    _TrackTime = _State->_TrackTime;
}

/// <summary>
/// Renders an X-axis (Time)
/// </summary>
void Spectogram::RenderXAxis(ID2D1RenderTarget * renderTarget, bool top) noexcept
{
    D2D1_RECT_F Rect = { 0.f, top ? 0.f : _BitmapSize.height, 0.f, top ? _XTextHeight : _Size.height };

    const FLOAT y1 = top ? _XTextHeight / 2.f : _BitmapSize.height;
    const FLOAT y2 = top ? _XTextHeight       : _BitmapSize.height + (_XTextHeight / 2.f);

    for (auto & Label : _XLabels)
    {
        // Draw the tick.
        renderTarget->DrawLine( { Label.X, y1 }, { Label.X, y2 }, _XAxisLineStyle->_Brush, _XAxisLineStyle->_Thickness);

        if (!_GraphSettings->_FlipHorizontally)
        {
            Rect.left  = Label.X + Offset;
            Rect.right = Rect.left + _XTextWidth;

            renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _XTextFormat, Rect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            if ((_State->_PlaybackTime != _PlaybackTime) && _State->_ScrollingSpectogram)
                Label.X--; // Scroll to the left.
        }
        else
        {
            Rect.left  = Label.X - Offset;
            Rect.right = Rect.left - _XTextWidth;

            renderTarget->DrawTextW(Label.Text.c_str(), (UINT32) Label.Text.size(), _XTextFormat, Rect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            if ((_State->_PlaybackTime != _PlaybackTime) && _State->_ScrollingSpectogram)
                Label.X++; // Scroll to the right.
        }
    }
}

/// <summary>
/// Updates this instance.
/// </summary>
void Spectogram::Update(const FrequencyBands & frequencyBands, double trackTime, double sampleRate) noexcept
{
    _BitmapRenderTarget->BeginDraw();

    if (_RequestErase)
    {
        _BitmapRenderTarget->Clear(D2D1::ColorF(0, 0.f)); // Make the bitmap completely transparent.

        _RequestErase = false;
    }

    // Draw the next spectogram line.
    {
        const FLOAT Bandwidth = Max((_BitmapSize.height / (FLOAT) _BandCount), 1.f);

        FLOAT y1 = 0.f;
        FLOAT y2 = Bandwidth;

        for (const auto & fb : frequencyBands)
        {
            if ((fb.Ctr >= (sampleRate / 2.)) && _State->_SuppressMirrorImage)
                break;

            assert(InRange(fb.CurValue, 0.0, 1.0));

            _SpectogramStyle->SetBrushColor(fb.CurValue);

            _BitmapRenderTarget->DrawLine({ (FLOAT) _X, y1 }, { (FLOAT) _X, y2 }, _SpectogramStyle->_Brush);

            y1  = y2;
            y2 += Bandwidth;
        }
    }

    if (_NyquistMarker->_ColorSource != ColorSource::None)
        RenderNyquistFrequencyMarker(_BitmapRenderTarget, frequencyBands, sampleRate);

    _BitmapRenderTarget->EndDraw();

    if (_TrackTime != trackTime)
        _XLabels.push_front({ pfc::wideFromUTF8(pfc::format_time((uint64_t) trackTime)),
             _GraphSettings->_FlipHorizontally ? (_State->_ScrollingSpectogram ? 0.f : _Size.width - (FLOAT) _X) :
                                                 (_State->_ScrollingSpectogram ? _Size.width : (FLOAT) _X) });
}

/// <summary>
/// Renders a marker for the Nyquist frequency.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void Spectogram::RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) const noexcept
{
    const double MinScale = ScaleF(frequencyBands.front().Ctr, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(frequencyBands.back() .Ctr, _State->_ScalingFunction, _State->_SkewFactor);

    const double NyquistScale = Clamp(ScaleF(sampleRate / 2., _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale);

    FLOAT y = Map(NyquistScale, MinScale, MaxScale, 0.f, _BitmapSize.height);

    renderTarget->DrawLine(D2D1_POINT_2F((FLOAT) _X, y), D2D1_POINT_2F((FLOAT) _X + 1, y), _NyquistMarker->_Brush, _NyquistMarker->_Thickness, nullptr);
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

    const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

    if (SUCCEEDED(hr) && (_XTextFormat == nullptr))
    {
        hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_XTextFormat);

        if (SUCCEEDED(hr))
        {
            _XTextFormat->SetTextAlignment(_GraphSettings->_FlipHorizontally ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);
            _XTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
            _XTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            CComPtr<IDWriteTextLayout> TextLayout;

            hr = _DirectWrite.Factory->CreateTextLayout(L"00:00", 5, _XTextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                // Calculate the metric.
                _XTextWidth  = TextMetrics.width;
                _XTextHeight = 2.f + TextMetrics.height + 2.f;
            }
        }
    }

    if (SUCCEEDED(hr) && (_YTextFormat == nullptr))
    {
        hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_YTextFormat);

        if (SUCCEEDED(hr))
        {
            _YTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);          // Right-aligned
            _YTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
            _YTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            CComPtr<IDWriteTextLayout> TextLayout;

            hr = _DirectWrite.Factory->CreateTextLayout(L"99.9fk", 6, _YTextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                // Calculate the metric.
                _YTextWidth  = TextMetrics.width;
                _YTextHeight = 2.f + TextMetrics.height + 2.f;
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Spectogram, renderTarget, _Size, &_SpectogramStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, renderTarget, _Size, &_XAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, _Size, &_XAxisTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, _Size, &_YAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, _Size, &_YAxisTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::NyquistMarker, renderTarget, _Size, &_NyquistMarker);

    _BitmapSize.width  = _Size.width;
    _BitmapSize.height = _Size.height - (_GraphSettings->_XAxisBottom ? _XTextHeight : 0.f) - (_GraphSettings->_XAxisTop ? _XTextHeight : 0.f);

    if (SUCCEEDED(hr) && (_BitmapRenderTarget == nullptr))
        hr = renderTarget->CreateCompatibleRenderTarget(_BitmapSize, &_BitmapRenderTarget);

    if (SUCCEEDED(hr) && (_Bitmap == nullptr))
        hr = _BitmapRenderTarget->GetBitmap(&_Bitmap);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Spectogram::ReleaseDeviceSpecificResources()
{
    if (_YAxisTextStyle)
    {
        _YAxisTextStyle->ReleaseDeviceSpecificResources();
        _YAxisTextStyle = nullptr;
    }

    if (_YAxisLineStyle)
    {
        _YAxisLineStyle->ReleaseDeviceSpecificResources();
        _YAxisLineStyle = nullptr;
    }

    if (_XAxisTextStyle)
    {
        _XAxisTextStyle->ReleaseDeviceSpecificResources();
        _XAxisTextStyle = nullptr;
    }

    if (_XAxisLineStyle)
    {
        _XAxisLineStyle->ReleaseDeviceSpecificResources();
        _XAxisLineStyle = nullptr;
    }

    if (_SpectogramStyle)
    {
        _SpectogramStyle->ReleaseDeviceSpecificResources();
        _SpectogramStyle = nullptr;
    }

    if (_NyquistMarker)
    {
        _NyquistMarker->ReleaseDeviceSpecificResources();
        _NyquistMarker = nullptr;
    }

    _YTextFormat.Release();
    _XTextFormat.Release();
    _Bitmap.Release();
    _BitmapRenderTarget.Release();
}

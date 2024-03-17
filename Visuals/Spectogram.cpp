
/** $VER: Spectogram.cpp (2024.03.17) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "Spectogram.h"

#include "Support.h"

#include "DirectWrite.h"

#pragma hdrstop

Spectogram::Spectogram()
{
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

    _Bitmap = nullptr;
    _BitmapRenderTarget = nullptr;

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

    _Bitmap = nullptr;
    _BitmapRenderTarget = nullptr;
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
        Update(frequencyBands, _State->_TrackTime);

    {
        D2D1::Matrix3x2F Transform = D2D1::Matrix3x2F::Identity();

        if (_GraphSettings->_FlipHorizontally)
        {
            const FLOAT Width = _Bounds.right - _Bounds.left;

            Transform = D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, Width, 0.f);
        }

        if (!_GraphSettings->_FlipVertically) // Negate because the GUI assumes the mathematical (bottom-left 0,0) coordinate system.
        {
            const FLOAT Height = _Bounds.bottom - _Bounds.top;

            const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, Height);

            Transform = Transform * FlipV;
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_Bounds.left, _Bounds.top);

        renderTarget->SetTransform(Transform * Translate);
    }

    // Draw the bitmap.
    {
        const bool Static = false;

        if (Static)
        {
            D2D1_RECT_F Rect = D2D1_RECT_F(0.f, 0.f, _BitmapSize.width, _BitmapSize.height);

            renderTarget->DrawBitmap(_Bitmap, &Rect);
        }
        else
        {
            D2D1_RECT_F Src = D2D1_RECT_F((FLOAT) (_X + 1), 0.f, _BitmapSize.width,              _BitmapSize.height);
            D2D1_RECT_F Dst = D2D1_RECT_F(             0.f, 0.f, _BitmapSize.width - (FLOAT) _X, _BitmapSize.height);

            renderTarget->DrawBitmap(_Bitmap, &Dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);

            Src.right = Src.left;
            Src.left  = 0.f;

            Dst.left  = Dst.right;
            Dst.right = _Size.width;

            renderTarget->DrawBitmap(_Bitmap, &Dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);
        }
    }

    {
        renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    }

    // Draw the X-axis (Time).
    if (!_XLabels.empty())
    {
        const FLOAT Offset = 4.f; // Distance between the tick and the text.

        for (auto & Label : _XLabels)
        {
            renderTarget->DrawLine( { Label.X, _BitmapSize.height }, { Label.X, _BitmapSize.height + (_XTextHeight / 2.f) }, _XAxisLineStyle->_Brush, 1.f); // Tick

            const D2D1_RECT_F Rect = { Label.X + Offset, _BitmapSize.height, Label.X + Offset + _XTextWidth, _Size.height };

            renderTarget->DrawTextW(Label.Text.c_str(), Label.Text.size(), _XTextFormat, Rect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            if (_State->_PlaybackTime != _PlaybackTime)
                Label.X--; // Scroll left.
        }

        if (_XLabels.back().X + Offset + _XTextWidth < 0.f)
            _XLabels.pop_back();
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

            Rect.top    = _BitmapSize.height - _YTextHeight - y;
            Rect.bottom = Rect.top +_YTextHeight;

            renderTarget->DrawTextW(Label.Text.c_str(), Label.Text.size(), _YTextFormat, Rect, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
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
/// Updates this instance.
/// </summary>
void Spectogram::Update(const FrequencyBands & frequencyBands, double trackTime) noexcept
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

        FLOAT y1 = _BitmapSize.height;
        FLOAT y2 = y1 - Bandwidth;

        for (const auto & fb : frequencyBands)
        {
            assert(InRange(fb.CurValue, 0.0, 1.0));

            _ForegroundStyle->SetBrushColor(fb.CurValue);

            _BitmapRenderTarget->DrawLine({ (FLOAT) _X, y1 }, { (FLOAT) _X, y2 }, _ForegroundStyle->_Brush);

            y1  = y2;
            y2 -= Bandwidth;
        }
    }

    _BitmapRenderTarget->EndDraw();

    if (_TrackTime != trackTime)
        _XLabels.push_front({ pfc::wideFromUTF8(pfc::format_time((uint64_t) trackTime)), _Size.width });
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
            _XTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);           // Left-aligned
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
    {
        if (_ForegroundStyle == nullptr)
            _ForegroundStyle = _State->_StyleManager.GetStyle(VisualElement::Spectogram);

        if (_ForegroundStyle && (_ForegroundStyle->_Brush == nullptr))
            hr = _ForegroundStyle->CreateDeviceSpecificResources(renderTarget, _Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_XAxisLineStyle == nullptr)
            _XAxisLineStyle = _State->_StyleManager.GetStyle(VisualElement::VerticalGridLine);

        if (_XAxisLineStyle && (_XAxisLineStyle->_Brush == nullptr))
            hr = _XAxisLineStyle->CreateDeviceSpecificResources(renderTarget, _Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_XAxisTextStyle == nullptr)
            _XAxisTextStyle = _State->_StyleManager.GetStyle(VisualElement::XAxisText);

        if (_XAxisTextStyle && (_XAxisTextStyle->_Brush == nullptr))
            hr = _XAxisTextStyle->CreateDeviceSpecificResources(renderTarget, _Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_YAxisLineStyle == nullptr)
            _YAxisLineStyle = _State->_StyleManager.GetStyle(VisualElement::HorizontalGridLine);

        if (_YAxisLineStyle && (_YAxisLineStyle->_Brush == nullptr))
            hr = _YAxisLineStyle->CreateDeviceSpecificResources(renderTarget, _Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_YAxisTextStyle == nullptr)
            _YAxisTextStyle = _State->_StyleManager.GetStyle(VisualElement::YAxisText);

        if (_YAxisTextStyle && (_YAxisTextStyle->_Brush == nullptr))
            hr = _YAxisTextStyle->CreateDeviceSpecificResources(renderTarget, _Size);
    }

    _BitmapSize.width  = _Size.width;
    _BitmapSize.height = _Size.height - _XTextHeight;

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

    if (_ForegroundStyle)
    {
        _ForegroundStyle->ReleaseDeviceSpecificResources();
        _ForegroundStyle = nullptr;
    }

    _YTextFormat.Release();
    _XTextFormat.Release();
    _Bitmap.Release();
    _BitmapRenderTarget.Release();
}

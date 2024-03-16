
/** $VER: HeatMap.cpp (2024.03.15) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "HeatMap.h"

#include "Support.h"

#include "DirectWrite.h"

#pragma hdrstop

HeatMap::HeatMap()
{
    _Size = { };

    _FontFamilyName = L"Segoe UI";
    _FontSize = 6.f;

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void HeatMap::Initialize(State * state, const GraphSettings * settings)
{
    _State = state;
    _GraphSettings = settings;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void HeatMap::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;
    _Size = { rect.right - rect.left, rect.bottom - rect.top };

    _Bitmap = nullptr;
    _RenderTarget = nullptr;
}

/// <summary>
/// Resets this instance.
/// </summary>
void HeatMap::Reset()
{
    _X = 0.;
    _ClearBackground = true;
    _OldTime = -1;

    _Labels.clear();

    _Bitmap = nullptr;
    _RenderTarget = nullptr;
}

/// <summary>
/// Renders the spectrum analysis as a heat map.
/// </summary>
void HeatMap::Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate, double time)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
    {
        const double dx = 1.;

        FLOAT BitmapHeight = _Size.height - _TextHeight;

        _RenderTarget->BeginDraw();

        if (_ClearBackground)
        {
            _RenderTarget->Clear(D2D1::ColorF(0, 0.f)); // Make the bitmap completely transparent.

            _ClearBackground = false;
        }

        {
            const FLOAT Bandwidth = Max((BitmapHeight / (FLOAT) frequencyBands.size()), 1.f);

            FLOAT y1 = BitmapHeight;
            FLOAT y2 = y1 - Bandwidth;

            for (const auto & fb : frequencyBands)
            {
                double Amplitude = _GraphSettings->ScaleA(fb.CurValue);

                if (::isfinite(Amplitude))
                {
                    _ForegroundStyle->SetBrushColor(Clamp(Amplitude, 0., 1.));

                    _RenderTarget->DrawLine({ (FLOAT) _X, y1 }, { (FLOAT) _X, y2 }, _ForegroundStyle->_Brush);
                }

                y1  = y2;
                y2 -= Bandwidth;
            }
        }

        _RenderTarget->EndDraw();

        if ((int) time != _OldTime)
        {
            _OldTime = (int) time;

            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%02d:%02d", _OldTime / 60, _OldTime % 60);

            _Labels.push_front({ Text, _Size.width });
        }

        // Draw the bitmap.
        {
            const bool Static = false;

            if (Static)
            {
                D2D1_RECT_F Rect = D2D1_RECT_F(0.f, 0.f, _Size.width, BitmapHeight);

                renderTarget->DrawBitmap(_Bitmap, &Rect);
            }
            else
            {
                D2D1_RECT_F Src = D2D1_RECT_F((FLOAT) (_X + 1.), 0.f, _Size.width,              BitmapHeight);
                D2D1_RECT_F Dst = D2D1_RECT_F(              0.f, 0.f, _Size.width - (FLOAT) _X, BitmapHeight);

                renderTarget->DrawBitmap(_Bitmap, &Dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);

                Src.left  = 0.f;
                Src.right = (FLOAT) (_X + 1.);

                Dst.left  = _Size.width - (FLOAT) _X;
                Dst.right = _Size.width;

                renderTarget->DrawBitmap(_Bitmap, &Dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);
            }
        }

        // Draw the X-axis.
        {
            const FLOAT Offset = 4.f;

            for (auto & Label : _Labels)
            {
                renderTarget->DrawLine( { Label._X, BitmapHeight }, { Label._X, BitmapHeight + (_TextHeight / 2.f) }, _Brush, 1.f); // Tick

                const D2D1_RECT_F Rect = { Label._X + Offset, BitmapHeight, Label._X + Offset + _TextWidth, _Size.height };

                renderTarget->DrawTextW(Label._Text.c_str(), Label._Text.size(), _TextFormat, Rect, _Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                Label._X--; // Scroll left.
            }

            if (_Labels.back()._X + Offset + _TextWidth < 0.f)
                _Labels.pop_back();
        }

        _X += dx;

        if (_X > (double) _Size.width)
            _X = 0.;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT HeatMap::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) && (_RenderTarget == nullptr))
        hr = renderTarget->CreateCompatibleRenderTarget(_Size, &_RenderTarget);

    if (SUCCEEDED(hr) && (_Bitmap == nullptr))
        hr = _RenderTarget->GetBitmap(&_Bitmap);

    if (SUCCEEDED(hr) && (_TextFormat == nullptr))
    {
        const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

        hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);           // Left-aligned
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
            _TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            CComPtr<IDWriteTextLayout> TextLayout;

            hr = _DirectWrite.Factory->CreateTextLayout(L"00:00", 5, _TextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                // Calculate the height.
                _TextWidth  = TextMetrics.width;
                _TextHeight = 2.f + TextMetrics.height + 2.f;
            }
        }
    }

    if (SUCCEEDED(hr) && (_Brush == nullptr))
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), (ID2D1SolidColorBrush **) &_Brush);

    if (SUCCEEDED(hr))
    {
        if (_ForegroundStyle == nullptr)
            _ForegroundStyle = _State->_StyleManager.GetStyle(VisualElement::HeatMapForeground);

        if (_ForegroundStyle && (_ForegroundStyle->_Brush == nullptr))
            hr = _ForegroundStyle->CreateDeviceSpecificResources(renderTarget, _Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_BackgroundStyle == nullptr)
            _BackgroundStyle = _State->_StyleManager.GetStyle(VisualElement::HeatMapBackground);

        if (_BackgroundStyle && (_BackgroundStyle->_Brush == nullptr))
            hr = _BackgroundStyle->CreateDeviceSpecificResources(renderTarget, _Size);
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void HeatMap::ReleaseDeviceSpecificResources()
{
    if (_BackgroundStyle)
    {
        _BackgroundStyle->ReleaseDeviceSpecificResources();
        _BackgroundStyle = nullptr;
    }

    if (_ForegroundStyle)
    {
        _ForegroundStyle->ReleaseDeviceSpecificResources();
        _ForegroundStyle = nullptr;
    }

    _Brush.Release();
    _TextFormat.Release();
    _Bitmap.Release();
    _RenderTarget.Release();
}

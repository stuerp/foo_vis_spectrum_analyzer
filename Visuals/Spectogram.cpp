
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
void Spectogram::Initialize(State * state, const GraphSettings * settings)
{
    _State = state;
    _GraphSettings = settings;

    ReleaseDeviceSpecificResources();
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
    _Time = std::numeric_limits<double>::max();
    _RequestErase = true;

    _Labels.clear();

    _Bitmap = nullptr;
    _BitmapRenderTarget = nullptr;
}

/// <summary>
/// Renders the spectrum analysis as a spectogram.
/// </summary>
void Spectogram::Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate, double time)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
    {
        const FLOAT BitmapHeight = _Size.height - _TextHeight;

        if (time != _Time)
        {
            _BitmapRenderTarget->BeginDraw();

            if (_RequestErase)
            {
                _BitmapRenderTarget->Clear(D2D1::ColorF(0, 0.f)); // Make the bitmap completely transparent.

                _RequestErase = false;
            }

            // Draw the next spectogram line.
            {
                const FLOAT Bandwidth = Max((BitmapHeight / (FLOAT) frequencyBands.size()), 1.f);

                FLOAT y1 = BitmapHeight;
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

            if ((uint64_t) _Time != (uint64_t) time)
                _Labels.push_front({ pfc::wideFromUTF8(pfc::format_time((uint64_t) time)), _Size.width - (FLOAT) (time - _Time) });
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
                D2D1_RECT_F Src = D2D1_RECT_F((FLOAT) (_X + 1), 0.f, _Size.width,              BitmapHeight);
                D2D1_RECT_F Dst = D2D1_RECT_F(             0.f, 0.f, _Size.width - (FLOAT) _X, BitmapHeight);

                renderTarget->DrawBitmap(_Bitmap, &Dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);

                Src.right = Src.left;
                Src.left  = 0.f;

                Dst.left  = Dst.right;
                Dst.right = _Size.width;

                renderTarget->DrawBitmap(_Bitmap, &Dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &Src);
            }
        }

        // Draw the X-axis.
        if (!_Labels.empty())
        {
            const FLOAT Offset = 4.f; // Distance between the tick and the text.

            for (auto & Label : _Labels)
            {
                renderTarget->DrawLine( { Label._X, BitmapHeight }, { Label._X, BitmapHeight + (_TextHeight / 2.f) }, _XAxisLineStyle->_Brush, 1.f); // Tick

                const D2D1_RECT_F Rect = { Label._X + Offset, BitmapHeight, Label._X + Offset + _TextWidth, _Size.height };

                renderTarget->DrawTextW(Label._Text.c_str(), Label._Text.size(), _TextFormat, Rect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                if (time != _Time)
                    Label._X--; // Scroll left.
            }

            if (_Labels.back()._X + Offset + _TextWidth < 0.f)
                _Labels.pop_back();
        }

        if (time != _Time)
        {
            _X++;

            if (_X > (uint32_t) _Size.width)
                _X = 0;
        }

        _Time = time;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT Spectogram::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) && (_BitmapRenderTarget == nullptr))
        hr = renderTarget->CreateCompatibleRenderTarget(_Size, &_BitmapRenderTarget);

    if (SUCCEEDED(hr) && (_Bitmap == nullptr))
        hr = _BitmapRenderTarget->GetBitmap(&_Bitmap);

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

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Spectogram::ReleaseDeviceSpecificResources()
{
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

    _TextFormat.Release();
    _Bitmap.Release();
    _BitmapRenderTarget.Release();
}

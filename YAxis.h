
/** $VER: YAxis.h (2023.11.15) P. Stuer - Represents and renders the Y axis. **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "Math.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the Y axis.
/// </summary>
class YAxis
{
public:
    YAxis() : _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _LabelWidth(30.f), _X(), _Y(), _ClientWidth(), _ClientHeight(), _TextHeight() { }

    YAxis(const YAxis &) = delete;
    YAxis & operator=(const YAxis &) = delete;
    YAxis(YAxis &&) = delete;
    YAxis & operator=(YAxis &&) = delete;

    FLOAT GetWidth() const { return _LabelWidth; }

    struct Label
    {
        std::wstring Text;
        FLOAT y;
    };

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    void Initialize(FLOAT x, FLOAT y, FLOAT width, FLOAT height)
    {
        _X = x;
        _Y = y;

        _ClientWidth = width;
        _ClientHeight = height;

        _Labels.clear();

        // Precalculate the labels and their position.
        {
            for (double Amplitude = _Configuration._MinDecibel; Amplitude <= _Configuration._MaxDecibel; Amplitude += 6.0)
            {
                y = (FLOAT) Map(ScaleA(ToMagnitude(Amplitude)), 0.0, 1.0, _ClientHeight, 0.0);

                WCHAR Text[16] = { };

                ::StringCchPrintfW(Text, _countof(Text), L"%ddB", (int) Amplitude);

                Label lb = { Text, y };

                _Labels.push_back(lb);
            }
        }
    }

    /// <summary>
    /// Renders this instance to the specified render target.
    /// </summary>
    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        const FLOAT StrokeWidth = 1.0f;

        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            {
                _Brush->SetColor(D2D1::ColorF(0x444444, 1.0f));

                renderTarget->DrawLine(D2D1_POINT_2F(_X + _LabelWidth, Iter.y), D2D1_POINT_2F(_ClientWidth, Iter.y), _Brush, StrokeWidth, nullptr);
            }

            // Draw the label.
            {
                D2D1_RECT_F TextRect = { _X, Iter.y - _TextHeight / 2.f, _X + _LabelWidth - 2.f, Iter.y + _TextHeight / 2.f };

                _Brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, _Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
            }
        }

        return S_OK;
    }

    /// <summary>
    /// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
    /// </summary>
    HRESULT CreateDeviceIndependentResources(CComPtr<IDWriteFactory> & directWriteFactory)
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr))
        {
            static const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

            hr = directWriteFactory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);
        }

        if (SUCCEEDED(hr))
        {
            CComPtr<IDWriteTextLayout> TextLayout;

            hr = directWriteFactory->CreateTextLayout(L"AaGg09", 6, _TextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);          // Right-align horizontally
                _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically

                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                _TextHeight = TextMetrics.height;
            }
            else
                Log(LogLevel::Critical, "%s: Unable to create Y axis TextLayout: 0x%08X.", core_api::get_my_file_name(), hr);
        }
        else
            Log(LogLevel::Critical, "%s: Unable to create Y axis TextFormat: 0x%08X.", core_api::get_my_file_name(), hr);

        return hr;
    }

    /// <summary>
    /// Creates resources which are bound to a particular D3D device.
    /// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
    /// </summary>
    HRESULT CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        if (_Brush != nullptr)
            return S_OK;

        HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_Brush);

        return hr;
    }

    /// <summary>
    /// Releases the device specific resources.
    /// </summary>
    void ReleaseDeviceSpecificResources()
    {
        _Brush.Release();
    }

private:
    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.
    FLOAT _LabelWidth;  // Determines the max. width of the label.

    // Parent-dependent parameters
    FLOAT _X;
    FLOAT _Y;

    FLOAT _ClientWidth;
    FLOAT _ClientHeight;

    std::vector<Label> _Labels;

    // Device-independent resources
    CComPtr<IDWriteTextFormat> _TextFormat;
    FLOAT _TextHeight;

    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _Brush;
};

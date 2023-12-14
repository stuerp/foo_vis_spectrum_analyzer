
/** $VER: YAxis.h (2023.12.10) P. Stuer - Represents and renders the Y axis. **/

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
    YAxis() : _Configuration(nullptr), _TextColor(), _LineColor(), _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _Rect(), _Width(30.f), _Height() { }

    YAxis(const YAxis &) = delete;
    YAxis & operator=(const YAxis &) = delete;
    YAxis(YAxis &&) = delete;
    YAxis & operator=(YAxis &&) = delete;

    struct Label
    {
        double Amplitude;
        std::wstring Text;
        FLOAT y;
    };

    /// <summary>
    /// Gets the width of the axis.
    /// </summary>
    FLOAT GetWidth() const { return _Width; }

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    void Initialize(const Configuration * configuration)
    {
        _Configuration = configuration;

        _TextColor = configuration->_YTextColor;
        _LineColor = configuration->_YLineColor;

        _Labels.clear();

        if (_Configuration->_YAxisMode == YAxisMode::None)
            return;

        // Precalculate the labels and their position.
        {
            for (double Amplitude = _Configuration->_AmplitudeLo; Amplitude <= _Configuration->_AmplitudeHi; Amplitude -= _Configuration->_AmplitudeStep)
            {
                WCHAR Text[16] = { };

                ::StringCchPrintfW(Text, _countof(Text), L"%ddB", (int) Amplitude);

                Label lb = { Amplitude, Text, 0.f };

                _Labels.push_back(lb);
            }
        }
    }

    /// <summary>
    /// Resizes the axis.
    /// </summary>
    void Resize(const D2D1_RECT_F & rect)
    {
        _Rect = rect;

        // Calculate the position of the labels based on the height.
        const FLOAT Height = _Rect.bottom - _Rect.top;

        for (Label & Iter : _Labels)
            Iter.y = Map(_Configuration->ScaleA(ToMagnitude(Iter.Amplitude)), 0.0, 1.0, Height, _Height / 2.f);
    }

    /// <summary>
    /// Renders this instance to the specified render target.
    /// </summary>
    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        if (_Configuration->_YAxisMode == YAxisMode::None)
            return S_OK;

        CreateDeviceSpecificResources(renderTarget);

        const FLOAT StrokeWidth = 1.0f;
        const FLOAT Width = _Rect.right - _Rect.left;

        FLOAT OldTextTop = _Rect.bottom - _Rect.top + _Height;

        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            {
                _Brush->SetColor(_Configuration->_UseCustomYLineColor ? _LineColor : ToD2D1_COLOR_F(_Configuration->_DefTextColor));

                renderTarget->DrawLine(D2D1_POINT_2F(_Rect.left + _Width, Iter.y), D2D1_POINT_2F(Width, Iter.y), _Brush, StrokeWidth, nullptr);
            }

            // Draw the label.
            {
                D2D1_RECT_F TextRect = { _Rect.left, Iter.y - (_Height / 2.f), _Rect.left + _Width - 2.f, Iter.y + (_Height / 2.f) };

                if (TextRect.bottom < OldTextTop)
                {
                    _Brush->SetColor(_Configuration->_UseCustomYTextColor ? _TextColor : ToD2D1_COLOR_F(_Configuration->_DefTextColor));

                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, _Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

                    OldTextTop = TextRect.top;
                }
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

            if (SUCCEEDED(hr))
            {
                _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);          // Right-align horizontally
                _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
            }
        }

        if (SUCCEEDED(hr))
        {
            CComPtr<IDWriteTextLayout> TextLayout;

            hr = directWriteFactory->CreateTextLayout(L"AaGg09", 6, _TextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                _Height = TextMetrics.height;
            }
        }

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
    const Configuration * _Configuration;

    std::vector<Label> _Labels;

    D2D1_COLOR_F _TextColor;
    D2D1_COLOR_F _LineColor;
    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    // Device-independent resources
    D2D1_RECT_F _Rect;
    FLOAT _Width;  // Width of the Y axis area (Font size-dependent).
    FLOAT _Height; // Height of a label

    CComPtr<IDWriteTextFormat> _TextFormat;

    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _Brush;
};

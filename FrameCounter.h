
/** $VER: FrameCounter.h (2023.11.15) P. Stuer - Represents and renders the frame counter display. **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "Math.h"
#include "RingBuffer.h"

#include <string>

/// <summary>
/// Implements the frame counter display.
/// </summary>
class FrameCounter
{
public:
    FrameCounter() : _Times(), _FontFamilyName(L"Segoe UI"), _FontSize(20.f), _LabelWidth(30.f), _ClientWidth(), _ClientHeight(), _TextWidth(), _TextHeight()
    {
        ::QueryPerformanceFrequency(&_Frequency);

        NewFrame();
    }

    FrameCounter(const FrameCounter &) = delete;
    FrameCounter & operator=(const FrameCounter &) = delete;
    FrameCounter(FrameCounter &&) = delete;
    FrameCounter & operator=(FrameCounter &&) = delete;

    FLOAT GetWidth() const { return _LabelWidth; }

    /// <summary>
    /// Registers the time at the time when starting a new frame.
    /// </summary>
    void NewFrame()
    {
        LARGE_INTEGER Counter;

        ::QueryPerformanceCounter(&Counter);

        _Times.Add(Counter.QuadPart);
    }

    float GetFPS()
    {
        float FPS = (float)((_Times.GetCount() - 1) * _Frequency.QuadPart) / (float) (_Times.GetLast() - _Times.GetFirst());

        return FPS;
    }

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    void Initialize(FLOAT clientWidth, FLOAT clientHeight)
    {
        _ClientWidth = clientWidth;
        _ClientHeight = clientHeight;
    }

    /// <summary>
    /// Renders this instance to the specified render target.
    /// </summary>
    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        WCHAR Text[512] = { };

        HRESULT hr = ::StringCchPrintfW(Text, _countof(Text), L"FPS: %.2f", GetFPS());

        if (SUCCEEDED(hr))
        {
            static const D2D1_RECT_F Rect = { _ClientWidth - 4.f - _TextWidth, 4.f, _ClientWidth - 4.f, 4.f + _TextHeight };
            static const FLOAT Inset = 4.f;

            // Draw the background.
            {
                _Brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f));

                renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _Brush);
            }

            // Draw the text.
            {
                _Brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

                renderTarget->DrawText(Text, (UINT) ::wcsnlen(Text, _countof(Text)), _TextFormat, Rect, _Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
            }
        }

        return hr;
    }

    /// <summary>
    /// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
    /// </summary>
    HRESULT CreateDeviceIndependentResources(CComPtr<IDWriteFactory> & directWriteFactory)
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr))
        {
            static const FLOAT FontSize = ToDIPs(_FontSize); // In DIPs

            hr = directWriteFactory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);
        }

        if (SUCCEEDED(hr))
        {
            const WCHAR Text[] = L"FPS: 999.99";

            CComPtr<IDWriteTextLayout> TextLayout;

            hr = directWriteFactory->CreateTextLayout(Text, _countof(Text), _TextFormat, 1920.f, 1080.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                _TextWidth  = TextMetrics.width;
                _TextHeight = TextMetrics.height;
            }
            else
                if (_Configuration._LogLevel <= LogLevel::Critical)
                    console::printf("%s: Unable to create Y axis TextLayout: 0x%08X.", core_api::get_my_file_name(), hr);
        }
        else
            if (_Configuration._LogLevel <= LogLevel::Critical)
                console::printf("%s: Unable to create Y axis TextFormat: 0x%08X.", core_api::get_my_file_name(), hr);

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
    LARGE_INTEGER _Frequency;
    RingBuffer<LONGLONG, 16> _Times;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.
    FLOAT _LabelWidth;  // Determines the max. width of the label.

    // Parent-dependent parameters
    FLOAT _ClientWidth;
    FLOAT _ClientHeight;

    // Device-independent resources
    CComPtr<IDWriteTextFormat> _TextFormat;
    FLOAT _TextWidth;
    FLOAT _TextHeight;

    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _Brush;
};

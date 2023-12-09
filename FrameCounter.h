
/** $VER: FrameCounter.h (2023.12.09) P. Stuer - Represents and renders the frame counter display. **/

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
    FrameCounter() : _Times(), _FontFamilyName(L"Segoe UI"), _FontSize(20.f), _ClientWidth(), _ClientHeight(), _TextWidth(), _TextHeight()
    {
        ::QueryPerformanceFrequency(&_Frequency);

        NewFrame();
    }

    FrameCounter(const FrameCounter &) = delete;
    FrameCounter & operator=(const FrameCounter &) = delete;
    FrameCounter(FrameCounter &&) = delete;
    FrameCounter & operator=(FrameCounter &&) = delete;

    void Resize(FLOAT clientWidth, FLOAT clientHeight);

    void NewFrame();
    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget);

    HRESULT CreateDeviceIndependentResources(CComPtr<IDWriteFactory> & directWriteFactory);

    HRESULT CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget);

    void ReleaseDeviceSpecificResources();

private:
    float GetFPS();

private:
    LARGE_INTEGER _Frequency;
    RingBuffer<LONGLONG, 16> _Times;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

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

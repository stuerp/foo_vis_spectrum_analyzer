
/** $VER: FrameCounter.h (2024.02.17) P. Stuer - Represents and renders the frame counter display. **/

#pragma once

#include "framework.h"
#include "Support.h"
#include "State.h"

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

    void Resize(FLOAT clientWidth, FLOAT clientHeight) noexcept;

    void NewFrame() noexcept;
    HRESULT Render(ID2D1RenderTarget * renderTarget) noexcept;

    HRESULT CreateDeviceIndependentResources() noexcept;
    void ReleaseDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    float GetFPS() const noexcept;

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

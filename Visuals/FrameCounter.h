
/** $VER: FrameCounter.h (2024.03.09) P. Stuer - Represents and renders the frame counter display. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>
#include <d2d1_2.h>

#include "Support.h"
#include "State.h"

#include "DirectWrite.h"
#include "RingBuffer.h"

#include <string>

/// <summary>
/// Implements the frame counter display.
/// </summary>
#pragma warning(disable: 4820)
class frame_counter_t
{
public:
    frame_counter_t() : _Times(), _FontFamilyName(L"Segoe UI"), _FontSize(20.f), _ClientWidth(), _ClientHeight(), _TextWidth(), _TextHeight()
    {
        ::QueryPerformanceFrequency(&_Frequency);

        NewFrame();
    }

    frame_counter_t(const frame_counter_t &) = delete;
    frame_counter_t & operator=(const frame_counter_t &) = delete;
    frame_counter_t(frame_counter_t &&) = delete;
    frame_counter_t & operator=(frame_counter_t &&) = delete;

    void Resize(FLOAT clientWidth, FLOAT clientHeight) noexcept;

    void NewFrame() noexcept;
    HRESULT Render(ID2D1DeviceContext * deviceContext) noexcept;

    HRESULT CreateDeviceIndependentResources() noexcept;
    void ReleaseDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    float GetFPS() const noexcept;

private:
    LARGE_INTEGER _Frequency;
    ring_buffer_t<LONGLONG, 16> _Times;

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

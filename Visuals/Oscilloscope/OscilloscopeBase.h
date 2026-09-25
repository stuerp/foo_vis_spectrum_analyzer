
/** $VER: OscilloscopeBase.h (2026.09.25) P. Stuer - Implements a base class for an oscilloscope. **/

#pragma once

#include <pch.h>

#include "Visualization.h"

class oscilloscope_base_t : public visualization_t
{
public:
    oscilloscope_base_t() = default;

    oscilloscope_base_t(const oscilloscope_base_t &) = delete;
    oscilloscope_base_t & operator=(const oscilloscope_base_t &) = delete;
    oscilloscope_base_t(oscilloscope_base_t &&) = delete;
    oscilloscope_base_t & operator=(oscilloscope_base_t &&) = delete;

    virtual ~oscilloscope_base_t() noexcept;

    // element_t
    virtual void Move(const D2D1_RECT_F & rect) noexcept = 0;
    virtual void Render(ID2D1DeviceContext * deviceContext, IDXGISwapChain1 * swapChain) noexcept = 0;
    virtual void Reset() noexcept = 0;

protected:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteSizeDependentResources() noexcept;

    HRESULT ClearBitmaps() noexcept;

    static size_t FindZeroCrossing(const audio_sample * frames, size_t frameCount, uint32_t channelCount) noexcept;

protected:
    size_t _PrevBitmapIndex = 1; // Start drawing in bitmap 0.

    bool _SquareBitmaps = false;
    D2D1_RECT_F _DestinationRectangle = { };

    style_t _SignalLineStyle;
    style_t _XAxisLineStyle;
    style_t _YAxisLineStyle;
    style_t _HorizontalGridLineStyle;
    style_t _VerticalGridLineStyle;

    // Device independent resources
    ComPtr<ID2D1StrokeStyle> _SignalStrokeStyle;
    ComPtr<ID2D1StrokeStyle1> _StaticStrokeStyle;

    // Device dependent resources
#ifdef _DEBUG
    ComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    ComPtr<ID2D1DeviceContext> _DeviceContext; // Device context used to render the phospor blur

    ComPtr<ID2D1Effect> _OpacityEffect;
    ComPtr<ID2D1Effect> _BlurEffect;

    // Device dependent resources (Size dependent)
    ComPtr<ID2D1Bitmap1> _Bitmaps[2];
};

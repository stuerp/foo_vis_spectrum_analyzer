
/** $VER: OscilloscopeBase.h (2026.06.21) P. Stuer - Implements a base class for an oscilloscope. **/

#pragma once

#include <pch.h>

#include "Visualization.h"

class oscilloscope_base_t : public visualization_t
{
public:
    oscilloscope_base_t();

    oscilloscope_base_t(const oscilloscope_base_t &) = delete;
    oscilloscope_base_t & operator=(const oscilloscope_base_t &) = delete;
    oscilloscope_base_t(oscilloscope_base_t &&) = delete;
    oscilloscope_base_t & operator=(oscilloscope_base_t &&) = delete;

    virtual ~oscilloscope_base_t() noexcept;

    // element_t
    virtual void Move(const D2D1_RECT_F & rect) noexcept = 0;
    virtual void Render(ID2D1DeviceContext * deviceContext) noexcept = 0;
    virtual void Reset() noexcept = 0;

    virtual void Resize() noexcept;

protected:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

protected:
    CComPtr<ID2D1StrokeStyle> _SignalStrokeStyle;
    CComPtr<ID2D1StrokeStyle1> _AxisStrokeStyle;

    style_t _SignalLineStyle;
    style_t _XAxisLineStyle;
    style_t _YAxisLineStyle;
    style_t _HorizontalGridLineStyle;
    style_t _VerticalGridLineStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    CComPtr<ID2D1DeviceContext> _DeviceContext; // Device context used to render the phospor blur

    CComPtr<ID2D1Bitmap1> _FrontBuffer;
    CComPtr<ID2D1Bitmap1> _BackBuffer;
    CComPtr<ID2D1Bitmap1> _CompositeBuffer;

    CComPtr<ID2D1Effect> _BlurEffect;
    CComPtr<ID2D1Effect> _ColorMatrixEffect;
};

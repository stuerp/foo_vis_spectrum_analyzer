
/** $VER: Oscilloscope.h (2025.10.14) P. Stuer - Implements an oscilloscope in X-Y mode. **/

#pragma once

#include <pch.h>

#include "Element.h"

class oscilloscope_xy_t : public element_t
{
public:
    oscilloscope_xy_t();

    oscilloscope_xy_t(const oscilloscope_xy_t &) = delete;
    oscilloscope_xy_t & operator=(const oscilloscope_xy_t &) = delete;
    oscilloscope_xy_t(oscilloscope_xy_t &&) = delete;
    oscilloscope_xy_t & operator=(oscilloscope_xy_t &&) = delete;

    virtual ~oscilloscope_xy_t();

    // element_t
    void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;

    void Resize() noexcept;

private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateGridCommandList() noexcept;

private:
    CComPtr<ID2D1StrokeStyle> _SignalStrokeStyle;
    CComPtr<ID2D1StrokeStyle1> _GridStrokeStyle;

    style_t * _SignalLineStyle;

    style_t * _XAxisTextStyle;
    style_t * _XAxisLineStyle;

    style_t * _YAxisTextStyle;
    style_t * _YAxisLineStyle;

    style_t * _HorizontalGridLineStyle;
    style_t * _VerticalGridLineStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    CComPtr<ID2D1DeviceContext> _DeviceContext; // Device context used to render the phospor blur

    CComPtr<ID2D1Bitmap1> _FrontBuffer;
    CComPtr<ID2D1Bitmap1> _BackBuffer;

    CComPtr<ID2D1Effect> _GaussBlurEffect;
    CComPtr<ID2D1Effect> _ColorMatrixEffect;

    CComPtr<ID2D1CommandList> _GridCommandList;
};

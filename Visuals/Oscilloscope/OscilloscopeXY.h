
/** $VER: Oscilloscope.h (2025.10.25) P. Stuer - Implements an oscilloscope in X-Y mode. **/

#pragma once

#include <pch.h>

#include "OscilloscopeBase.h"

class oscilloscope_xy_t : public oscilloscope_base_t
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
    style_t * _XAxisTextStyle;
    style_t * _YAxisTextStyle;

    CComPtr<ID2D1CommandList> _GridCommandList;
};

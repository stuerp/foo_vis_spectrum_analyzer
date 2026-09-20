
/** $VER: OscilloscopeXY.h (2026.06.17) P. Stuer - Implements an oscilloscope in X-Y mode. **/

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

    virtual ~oscilloscope_xy_t() noexcept;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext, CComPtr<IDXGISwapChain1> swapChain) noexcept override final;
    void Reset() noexcept override final;

    void Configure(state_t * state, graph_options_t * graphOptions, const analysis_t * analysis, bool isFirst, bool isLast, CComPtr<ID3D11Device> d3dDevice = nullptr, CComPtr<ID3D11DeviceContext> d3dDeviceContext = nullptr) noexcept override final;
    void Resize() noexcept;

private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateGridCommandList() noexcept;

private:
    style_t _XAxisTextStyle;
    style_t _YAxisTextStyle;

    CComPtr<ID2D1CommandList> _GridCommandList;
};

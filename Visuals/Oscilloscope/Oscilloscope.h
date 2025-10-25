
/** $VER: Oscilloscope.h (2025.10.25) P. Stuer - Implements an oscilloscope. **/

#pragma once

#include <pch.h>

#include "OscilloscopeBase.h"

class oscilloscope_t : public oscilloscope_base_t
{
public:
    oscilloscope_t();

    oscilloscope_t(const oscilloscope_t &) = delete;
    oscilloscope_t & operator=(const oscilloscope_t &) = delete;
    oscilloscope_t(oscilloscope_t &&) = delete;
    oscilloscope_t & operator=(oscilloscope_t &&) = delete;

    virtual ~oscilloscope_t();

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

    HRESULT CreateSignalGeometry(const D2D1_SIZE_F & size, CComPtr<ID2D1PathGeometry> & geometry) noexcept;
    HRESULT CreateAxesCommandList() noexcept;

private:
    struct label_t
    {
        std::wstring Text;
        double Amplitude;

        bool IsMin;
        bool IsMax;
    };

    std::vector<label_t> _Labels;

    style_t * _XAxisTextStyle;
    style_t * _YAxisTextStyle;

    CComPtr<ID2D1CommandList> _AxesCommandList;
};

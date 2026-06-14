
/** $VER: BitMeter.h (2026.06.10) P. Stuer - Implements a bit meter visualization. **/

#pragma once

#include <pch.h>

#include "Visualization.h"

class bit_meter_t : public visualization_t
{
public:
    bit_meter_t();

    bit_meter_t(const bit_meter_t &) = delete;
    bit_meter_t & operator=(const bit_meter_t &) = delete;
    bit_meter_t(bit_meter_t &&) = delete;
    bit_meter_t & operator=(bit_meter_t &&) = delete;

    virtual ~bit_meter_t() noexcept;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;
    void Release() noexcept override final;

    // visualization_t
    void Initialize(state_t * state, graph_description_t * graphDescription, const analysis_t * analysis) noexcept override final;
    void Resize() noexcept;

    void OnConfigurationChange(ConfigurationChanges configurationChanges) noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(_In_ ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateStaticContentCommandList() noexcept;

private:
    size_t _MeasurementCount;
    size_t _BitCount;

    std::vector<std::wstring> _Labels;

    style_t * _BarBackground;

    style_t * _BarSign;
    style_t * _BarMantissa;
    style_t * _BarExponent;
    style_t * _XAxisText;
    style_t * _YAxisText;

    std::vector<style_t *> _Styles;

    CComPtr<ID2D1SolidColorBrush> _DebugBrush;

    CComPtr<ID2D1DeviceContext> _DeviceContext;
    CComPtr<ID2D1CommandList> _StaticContentCommandList;

    const FLOAT XPadding = 2.f;
    const FLOAT YPadding = 2.f;

    #if (audio_sample_size == 64)
    const size_t ExponentBits = 11;
    #else
    const size_t ExponentBits =  8;
    #endif
};

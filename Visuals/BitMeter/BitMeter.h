
/** $VER: BitMeter.h (2026.10.10) P. Stuer - Implements a bit meter visualization. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include "Visualization.h"

class bit_meter_t : public visualization_t
{
public:
    bit_meter_t() = default;

    bit_meter_t(const bit_meter_t &) = delete;
    bit_meter_t & operator=(const bit_meter_t &) = delete;
    bit_meter_t(bit_meter_t &&) = delete;
    bit_meter_t & operator=(bit_meter_t &&) = delete;

    virtual ~bit_meter_t() noexcept;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;

    void OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept override final;

    // visualization_t
    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(_In_ ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteSizeDependentResources() noexcept;

    HRESULT CreateColorDependentResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteColorDependentResources() noexcept;

    HRESULT CreateStaticContentCommandList() noexcept;

    void Initialize() noexcept;

private:
    size_t _MeasurementCount { 0 };
    size_t _BitCount { 0 };

    std::vector<std::wstring> _Labels;

    style_t _BarBackground;
    style_t _BarSign;
    style_t _BarMantissa;
    style_t _BarExponent;

    style_t _XAxisText;
    style_t _YAxisText;

    std::vector<style_t *> _Styles;
    BitMeterMode _CachedBitMeterMode = BitMeterMode::Integer;

    ComPtr<ID2D1DeviceContext> _DeviceContext;
    ComPtr<ID2D1CommandList> _StaticContentCommandList;
    ComPtr<ID2D1SolidColorBrush> _DebugBrush;

    static constexpr FLOAT XPadding = 2.f;
    static constexpr FLOAT YPadding = 2.f;

    #if (audio_sample_size == 64)
    static constexpr size_t ExponentBits = 11;
    #else
    static constexpr size_t ExponentBits =  8;
    #endif
};

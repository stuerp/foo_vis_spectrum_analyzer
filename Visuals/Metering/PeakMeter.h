
/** $VER: PeakMeter.h (2026.10.28) P. Stuer - Represents a peak meter. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include "Visualization.h"
#include "MeteringProcessor.h"
#include "PeakMeterParts.h"

namespace Metering
{

class peak_meter_t final : public visualization_t
{
public:
    peak_meter_t(state_t & state) : visualization_t(state) { }

    peak_meter_t(const peak_meter_t &) = delete;
    peak_meter_t(peak_meter_t &&) = delete;

    peak_meter_t & operator=(const peak_meter_t &) = delete;
    peak_meter_t & operator=(peak_meter_t &&) = delete;

    virtual ~peak_meter_t() noexcept;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;

    void OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept;

    // visualization_t
    void Configure(graph_options_t * options, audio_processor_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteSizeDependentResources() noexcept;

    HRESULT CreateColorDependentResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteColorDependentResources() noexcept;

    HRESULT CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept;

    void CreateParts() noexcept;
    void DeleteParts() noexcept;

    void MeasureParts(ID2D1DeviceContext * deviceContext) noexcept;

    void Initialize() noexcept;

private:
    processor_t * _Processor = nullptr;

    uint32_t _RenderedChannels = 0;

    static constexpr FLOAT _TickSize = 4.f;

    style_t _BarBackgroundStyle;

    style_t _PeakStyle;
    style_t _Peak0dBStyle;
    style_t _MaxPeakStyle;
    style_t _PeakTextStyle;

    style_t _RMSStyle;
    style_t _RMS0dBStyle;
    style_t _RMSTextStyle;

    style_t _NameStyle;

    style_t _ScaleTextStyle;
    style_t _ScaleLineStyle;

    std::vector<std::unique_ptr<part_t>> _Parts;

    size_t _BarCount { 0 }; // Number bars in the part list.

private:
    ComPtr<ID2D1Bitmap> _OpacityMask;
    ComPtr<ID2D1SolidColorBrush> _DebugBrush;
};

}

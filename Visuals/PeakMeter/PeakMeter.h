
/** $VER: PeakMeter.h (2025.11.12) P. Stuer - Represents a peak meter. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include <atlbase.h>

#include "Element.h"
#include "PeakMeterParts.h"

class peak_meter_t : public element_t
{
public:
    peak_meter_t();

    peak_meter_t(const peak_meter_t &) = delete;
    peak_meter_t & operator=(const peak_meter_t &) = delete;
    peak_meter_t(peak_meter_t &&) = delete;
    peak_meter_t & operator=(peak_meter_t &&) = delete;

    virtual ~peak_meter_t() noexcept;

    // element_t
    void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept;

    void CreateParts() noexcept;
    void DeleteParts() noexcept;

    void MeasureParts(ID2D1DeviceContext * deviceContext) noexcept;

private:
    uint32_t _RenderedChannels;

    const FLOAT _TickSize = 4.f;

    style_t * _BackgroundStyle;

    style_t * _PeakStyle;
    style_t * _Peak0dBStyle;
    style_t * _MaxPeakStyle;
    style_t * _PeakTextStyle;

    style_t * _RMSStyle;
    style_t * _RMS0dBStyle;
    style_t * _RMSTextStyle;

    style_t * _NameStyle;

    style_t * _ScaleTextStyle;
    style_t * _ScaleLineStyle;

    CComPtr<ID2D1Bitmap> _OpacityMask;

    CComPtr<ID2D1SolidColorBrush> _DebugBrush;

    std::vector<part_t *> _Parts;
};

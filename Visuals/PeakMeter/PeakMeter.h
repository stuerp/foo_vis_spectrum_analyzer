
/** $VER: PeakMeter.h (2025.09.24) P. Stuer - Represents a peak meter. **/

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
#include "PeakMeterTypes.h"

#include "Gauges.h"
#include "GaugeScales.h"
#include "GaugeNames.h"
#include "RMSReadOut.h"
#include "PeakReadOut.h"

class peak_meter_t : public element_t
{
public:
    peak_meter_t();

    peak_meter_t(const peak_meter_t &) = delete;
    peak_meter_t & operator=(const peak_meter_t &) = delete;
    peak_meter_t(peak_meter_t &&) = delete;
    peak_meter_t & operator=(peak_meter_t &&) = delete;

    virtual ~peak_meter_t();

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1DeviceContext * deviceContext) noexcept;
    void Reset() noexcept;

    void Resize() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    gauge_metrics_t _GaugeMetrics;

    gauge_t      _Gauges;
    gauge_scales_t _GaugeScales;
    gauge_names_t  _GaugeNames;
    rms_read_out_t  _RMSReadOut;
    peak_read_out_t _PeakReadOut;

    D2D1::Matrix3x2F _GaugesTransform;
    D2D1::Matrix3x2F _GaugeScalesTransform;
    D2D1::Matrix3x2F _GaugeNamesTransform;
    D2D1::Matrix3x2F _RMSReadOutTransform;
    D2D1::Matrix3x2F _PeakReadOutTransform;
};


/** $VER: Gauges.h (2024.04.22) P. Stuer - Implements a gauge of the peak meter. **/

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

class gauge_t : public element_t
{
public:
    gauge_t() { };

    gauge_t(const gauge_t &) = delete;
    gauge_t & operator=(const gauge_t &) = delete;
    gauge_t(gauge_t &&) = delete;
    gauge_t & operator=(gauge_t &&) = delete;

    virtual ~gauge_t() { }

    // element_t
    void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final { };
    void Reset() noexcept override final;

    void Resize() noexcept;
    void Render(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    bool GetMetrics(gauge_metrics_t & gaugeMetrics) const noexcept;

private:
    HRESULT CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept;

private:
    CComPtr<ID2D1Bitmap> _OpacityMask;

    style_t * _BackgroundStyle;

    style_t * _PeakStyle;
    style_t * _Peak0dBStyle;
    style_t * _MaxPeakStyle;

    style_t * _RMSStyle;
    style_t * _RMS0dBStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

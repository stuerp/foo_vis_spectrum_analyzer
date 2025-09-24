
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

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget) noexcept { };
    void Reset() noexcept;

    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

    bool GetMetrics(gauge_metrics_t & gaugeMetrics) const noexcept;

private:
    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

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

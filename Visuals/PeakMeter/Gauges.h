
/** $VER: Gauges.h (2024.04.22) P. Stuer - Implements the gauges of the peak meter. **/

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

class Gauges : public Element
{
public:
    Gauges() { };

    Gauges(const Gauges &) = delete;
    Gauges & operator=(const Gauges &) = delete;
    Gauges(Gauges &&) = delete;
    Gauges & operator=(Gauges &&) = delete;

    virtual ~Gauges() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

    bool GetMetrics(GaugeMetrics & gaugeMetrics) const noexcept;

private:
    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

private:
    CComPtr<ID2D1Bitmap> _OpacityMask;

    Style * _BackgroundStyle;

    Style * _PeakStyle;
    Style * _Peak0dBStyle;
    Style * _MaxPeakStyle;

    Style * _RMSStyle;
    Style * _RMS0dBStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

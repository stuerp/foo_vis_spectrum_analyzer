
/** $VER: PeakMeter.h (2024.04.22) P. Stuer - Represents a peak meter. **/

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

class PeakMeter : public Element
{
public:
    PeakMeter();

    PeakMeter(const PeakMeter &) = delete;
    PeakMeter & operator=(const PeakMeter &) = delete;
    PeakMeter(PeakMeter &&) = delete;
    PeakMeter & operator=(PeakMeter &&) = delete;

    virtual ~PeakMeter() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    GaugeMetrics _GaugeMetrics;

    Gauges      _Gauges;
    GaugeScales _GaugeScales;
    GaugeNames  _GaugeNames;
    RMSReadOut  _RMSReadOut;
    PeakReadOut _PeakReadOut;

    D2D1::Matrix3x2F _GaugesTransform;
    D2D1::Matrix3x2F _GaugeScalesTransform;
    D2D1::Matrix3x2F _GaugeNamesTransform;
    D2D1::Matrix3x2F _RMSReadOutTransform;
    D2D1::Matrix3x2F _PeakReadOutTransform;
};

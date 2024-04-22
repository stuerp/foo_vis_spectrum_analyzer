
/** $VER: CorrelationMeter.h (2024.04.22) P. Stuer - Represents a correlation meter. **/

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

class CorrelationMeter : public Element
{
public:
    CorrelationMeter();

    CorrelationMeter(const CorrelationMeter &) = delete;
    CorrelationMeter & operator=(const CorrelationMeter &) = delete;
    CorrelationMeter(CorrelationMeter &&) = delete;
    CorrelationMeter & operator=(CorrelationMeter &&) = delete;

    virtual ~CorrelationMeter() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
//  Gauges      _Gauges;
//  D2D1::Matrix3x2F _GaugesTransform;
};


/** $VER: HeatMap.h (2024.03.15) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include <atlbase.h>

#include "Element.h"
#include "FrequencyBand.h"

class HeatMap : public Element
{
public:
    HeatMap();

    HeatMap(const HeatMap &) = delete;
    HeatMap & operator=(const HeatMap &) = delete;
    HeatMap(HeatMap &&) = delete;
    HeatMap & operator=(HeatMap &&) = delete;

    void Initialize(State * state, const GraphSettings * settings);
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate);

private:
    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

private:
    D2D1_SIZE_F _Size;
    double _X;

    CComPtr<ID2D1BitmapRenderTarget> _RenderTarget;
};

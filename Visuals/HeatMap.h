
/** $VER: HeatMap.h (2024.03.15) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include <atlbase.h>

class HeatMap
{
private:
    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

private:
    CComPtr<ID2D1BitmapRenderTarget> _HeatmapRenderTarget;
};

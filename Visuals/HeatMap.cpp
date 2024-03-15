
/** $VER: HeatMap.cpp (2024.03.15) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#include "HeatMap.h"

#pragma hdrstop

/// <summary>
/// Creates a render target to render the heat map in.
/// </summary>
HRESULT HeatMap::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    D2D1_SIZE_F Size = { _Bounds.right - _Bounds.left, _Bounds.bottom - _Bounds.top };

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(Size, &_HeatmapRenderTarget);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void HeatMap::ReleaseDeviceSpecificResources()
{
    _HeatmapRenderTarget.Release();
}

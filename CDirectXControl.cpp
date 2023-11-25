
/** $VER: CDirectXControl.cpp (2023.11.25) P. Stuer - Implements a base class for DirectX rendered controls. **/

#include "framework.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "comdlg32")

#include "CDirectXControl.h"

#include "Support.h"

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT CDirectXControl::CreateDeviceIndependentResources()
{
    HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_Direct2dFactory);

    return hr;
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT CDirectXControl::CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size)
{
    if (_Direct2dFactory == nullptr)
        return E_FAIL;

    HRESULT hr = S_OK;

    // Create the render target.
    if (_RenderTarget == nullptr)
    {
        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(hWnd, size);

        hr = _Direct2dFactory->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);

        if (SUCCEEDED(hr))
            _RenderTarget->SetAntialiasMode(_UseAntialiasing ? D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void CDirectXControl::ReleaseDeviceSpecificResources()
{
    _RenderTarget.Release();
}


/** $VER: CDirectXControl.cpp (2026.09.26) P. Stuer - Implements a base class for DirectX rendered controls. **/

#include "pch.h"

#include "DirectXControl.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "comdlg32")

#include <Direct2D.h>

#pragma hdrstop

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the window.
/// </summary>
HRESULT directx_control_t::CreateDeviceIndependentResources() noexcept
{
    return S_OK;
}

/// <summary>
/// Deletes the resources which are not bound to any D3D device.
/// </summary>
void directx_control_t::DeleteDeviceIndependentResources() noexcept
{
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT directx_control_t::CreateDeviceSpecificResources() noexcept
{
    HRESULT hr = S_OK;

    // Create the render target.
    if (_RenderTarget == nullptr)
    {
        CRect rc;

        ::GetClientRect(_hWnd, &rc);

        D2D1_SIZE_U Size = D2D1::SizeU((UINT32) rc.Width(), (UINT32) rc.Height());

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(_UseHardwareRendering ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE);
        D2D1_HWND_RENDER_TARGET_PROPERTIES WindowRenderTargetProperties = D2D1::HwndRenderTargetProperties(_hWnd, Size);

        hr = Direct2DFactory::Get()->CreateHwndRenderTarget(RenderTargetProperties, WindowRenderTargetProperties, &_RenderTarget);

        if (SUCCEEDED(hr))
            _RenderTarget->SetAntialiasMode(_UseAntialiasing ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void directx_control_t::DeleteDeviceSpecificResources() noexcept
{
    _RenderTarget.Reset();
}

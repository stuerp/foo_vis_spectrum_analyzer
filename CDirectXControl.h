
/** $VER: CDirectXControl.h (2023.11.25) P. Stuer - Implements a base class for DirectX rendered controls. **/

#pragma once

#include "framework.h"

class CDirectXControl
{
public:

protected:
    virtual void OnSize(UINT type, CSize size)
    {
        ReleaseDeviceSpecificResources();
    }

    #pragma region DirectX
    virtual HRESULT CreateDeviceIndependentResources();
    virtual HRESULT CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size);
    virtual void ReleaseDeviceSpecificResources();
    #pragma endregion

protected:
    // Device-independent resources
    CComPtr<ID2D1Factory> _Direct2dFactory;

    const bool _UseHardwareRendering = true;
    const bool _UseAntialiasing = true;

    // Device-specific resources
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;
};

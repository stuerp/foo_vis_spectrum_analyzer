
/** $VER: CDirectXControl.h (2023.12.31) P. Stuer - Implements a base class for DirectX rendered controls. **/

#pragma once

#include "framework.h"

class CDirectXControl
{
public:
    CDirectXControl() { };

    CDirectXControl(const CDirectXControl &) = delete;
    CDirectXControl & operator=(const CDirectXControl &) = delete;
    CDirectXControl(CDirectXControl &&) = delete;
    CDirectXControl & operator=(CDirectXControl &&) = delete;

    virtual ~CDirectXControl() { ReleaseDeviceIndependentResources(); }

protected:
    virtual void OnSize(UINT type, CSize size)
    {
        ReleaseDeviceSpecificResources();
    }

    #pragma region DirectX

    virtual HRESULT CreateDeviceIndependentResources();
    virtual void ReleaseDeviceIndependentResources();

    virtual HRESULT CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size);
    virtual void ReleaseDeviceSpecificResources();

    #pragma endregion

protected:
    // Device-independent resources
    CComPtr<ID2D1Factory2> _Direct2D;

    const bool _UseHardwareRendering = true;
    const bool _UseAntialiasing = true;

    // Device-specific resources
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;
};

#define NM_CHANGED (NM_RETURN)

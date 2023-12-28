
/** $VER: CDirectXControl.h (2023.11.26) P. Stuer - Implements a base class for DirectX rendered controls. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

class CDirectXControl
{
public:
    CDirectXControl() { };

    CDirectXControl(const CDirectXControl &) = delete;
    CDirectXControl & operator=(const CDirectXControl &) = delete;
    CDirectXControl(CDirectXControl &&) = delete;
    CDirectXControl & operator=(CDirectXControl &&) = delete;

    virtual ~CDirectXControl() { }

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
    CComPtr<ID2D1Factory> _Direct2dFactory;

    const bool _UseHardwareRendering = true;
    const bool _UseAntialiasing = true;

    // Device-specific resources
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;
};

#define NM_CHANGED (NM_RETURN)

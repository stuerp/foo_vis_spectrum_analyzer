
/** $VER: CDirectXControl.h (2024.01.21) P. Stuer - Implements a base class for DirectX rendered controls. **/

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
        if (_RenderTarget == nullptr)
            return;

        D2D1_SIZE_U Size = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

        _RenderTarget->Resize(Size);
    }

    #pragma region DirectX

    virtual HRESULT CreateDeviceIndependentResources();
    virtual void ReleaseDeviceIndependentResources();

    virtual HRESULT CreateDeviceSpecificResources();
    virtual void ReleaseDeviceSpecificResources();

    #pragma endregion

protected:
    HWND _hWnd;

    // Device-independent resources
    CComPtr<ID2D1Factory2> _Direct2D;

    const bool _UseHardwareRendering = true;
    const bool _UseAntialiasing = true;

    // Device-specific resources
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;
};

#define NM_CHANGED (NM_RETURN)

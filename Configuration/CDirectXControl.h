
/** $VER: CDirectXControl.h (2024.03.09) P. Stuer - Implements a base class for DirectX rendered controls. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <d2d1_2.h>
#include <d2d1helper.h>

#include <atlbase.h>
#include <atltypes.h>
#include <atlstr.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>
#include <atlcom.h>
#include <atlcrack.h>

class CDirectXControl
{
public:
    CDirectXControl() { };

    CDirectXControl(const CDirectXControl &) = delete;
    CDirectXControl & operator=(const CDirectXControl &) = delete;
    CDirectXControl(CDirectXControl &&) = delete;
    CDirectXControl & operator=(CDirectXControl &&) = delete;

    virtual ~CDirectXControl() { DeleteDeviceIndependentResources(); }

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
    virtual void DeleteDeviceIndependentResources();

    virtual HRESULT CreateDeviceSpecificResources();
    virtual void DeleteDeviceSpecificResources();

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

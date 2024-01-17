
/** $VER: DirectX.cpp (2024.01.15) P. Stuer **/

#include "DirectX.h"
#include "Direct2D.h"

#include "SafeModuleHandle.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
DirectX::DirectX()
{
}

/// <summary>
/// Gets the DPI setting of the specified window.
/// </summary>
HRESULT DirectX::GetDPI(HWND hWnd, UINT & dpi) const
{
    SafeModuleHandle Module = SafeModuleHandle(L"user32.dll");

    typedef UINT (WINAPI * GetDpiForWindow_t)(_In_ HWND hwnd);

    GetDpiForWindow_t GetDpiForWindow_ = (GetDpiForWindow_t) Module.GetFunctionAddress("GetDpiForWindow");

    if (GetDpiForWindow_ != nullptr)
        dpi = GetDpiForWindow_(hWnd);
    else
    {
        FLOAT DPIX, DPIY;

        #pragma warning(disable: 4996)
        _Direct2D.Factory->GetDesktopDpi(&DPIX, &DPIY);
        #pragma warning(default: 4996)

        dpi = (UINT) DPIX;
    }

    return S_OK;
}

DirectX _DirectX;

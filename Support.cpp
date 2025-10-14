
/** $VER: Support.cpp (2024.03.09) P. Stuer **/

#include "pch.h"
#include "Support.h"

#include "Direct2D.h"
#include "SafeModuleHandle.h"

#pragma hdrstop

/// <summary>
/// Gets the DPI setting of the specified window.
/// </summary>
HRESULT GetDPI(HWND hWnd, UINT & dpi)
{
    safe_module_handle_t Module = safe_module_handle_t(L"user32.dll");

    typedef UINT (WINAPI * GetDpiForWindow_t)(_In_ HWND hwnd);

    const auto GetDpiForWindow_ = (GetDpiForWindow_t) Module.GetFunctionAddress("GetDpiForWindow");

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

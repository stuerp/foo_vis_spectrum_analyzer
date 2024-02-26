
/** $VER: Support.cpp (2024.02.26) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Direct2D.h"

#include "SafeModuleHandle.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Gets the DPI setting of the specified window.
/// </summary>
HRESULT GetDPI(HWND hWnd, UINT & dpi)
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

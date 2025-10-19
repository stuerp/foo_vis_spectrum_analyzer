
/** $VER: Support.cpp (2025.10.20) P. Stuer **/

#include "pch.h"
#include "Support.h"

#include "Direct2D.h"
#include "SafeModuleHandle.h"

#include <shellscalingapi.h>

#pragma comment(lib, "shcore")

#pragma hdrstop

/// <summary>
/// Initialize our DPI awareness.
/// </summary>
HRESULT InitializeDpiAwareness() noexcept
{
    return ::SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE); // Stick with Windows 8.1 for now.
}

/// <summary>
/// Gets the DPI setting of the specified window.
/// </summary>
HRESULT GetDPI(_In_ HWND hWnd, _Out_ UINT & dpi) noexcept
{
    safe_module_handle_t Module = safe_module_handle_t(L"user32.dll");

    typedef UINT (WINAPI * GetDpiForWindow_t)(_In_ HWND hwnd);

    const auto GetDpiForWindow_ = (GetDpiForWindow_t) Module.GetFunctionAddress("GetDpiForWindow"); // Windows 10 or higher

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

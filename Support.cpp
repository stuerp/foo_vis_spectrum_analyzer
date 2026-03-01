
/** $VER: Support.cpp (2026.03.01) P. Stuer **/

#include "pch.h"

#include "Support.h"
#include "CustomTitleformatHook.h"

#include "Direct2D.h"
#include "SafeModuleHandle.h"

#include <shellscalingapi.h>

#pragma comment(lib, "shcore")

//#include <pfc/string_conv.h>
//#include <pfc/string-conv-lite.h>

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
        CComPtr<ID2D1Factory2> D2DFactory;

        D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_NONE };

        HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, Options, &D2DFactory);

        if (SUCCEEDED(hr))
        {
            FLOAT DPIX, DPIY;

            #pragma warning(disable: 4996) // 'ID2D1Factory::GetDesktopDpi': Deprecated.
            D2DFactory->GetDesktopDpi(&DPIX, &DPIY);
            #pragma warning(default: 4996)

            dpi = (UINT) DPIX;
        }
        else
            dpi = 72;
    }

    return S_OK;
}

/// <summary>
/// Evaluates a foobar2000 Title Format script.
/// </summary>
HRESULT EvaluateTitleFormatScript(_In_ const std::wstring & script, _Out_ pfc::string & result) noexcept
{
    service_ptr_t<titleformat_object> tfo;

    static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(tfo, pfc::utf8FromWide(script.c_str()));

    custom_titleformat_hook_t Hook;

    tfo->run(&Hook, result, nullptr);

    return S_OK;
}

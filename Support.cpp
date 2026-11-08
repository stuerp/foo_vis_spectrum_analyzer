
/** $VER: Support.cpp (2026.11.08) P. Stuer **/

#include "pch.h"

#include "Support.h"
#include "CustomTitleformatHook.h"
#include "Log.h"

#include "Module.h"

#include <windef.h>
#include <d2d1_2.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

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
HRESULT GetDPI(HWND hWnd, UINT & dpi) noexcept
{
    auto Module = module_t(L"user32.dll");

    {
        typedef DPI_AWARENESS_CONTEXT (WINAPI * GetThreadDpiAwarenessContext_t)(HWND hwnd);

        #pragma warning(disable: 4191) // 'type cast': unsafe conversion from 'FARPROC' to 'GetDPI::GetThreadDpiAwarenessContext_'
        const auto GetThreadDpiAwarenessContext_ = (GetThreadDpiAwarenessContext_t) Module.GetFunctionAddress("GetThreadDpiAwarenessContext"); // Windows 10 or higher

        if (GetThreadDpiAwarenessContext_ != nullptr)
        {
            const DPI_AWARENESS_CONTEXT Context = GetThreadDpiAwarenessContext_(hWnd);

            switch ((size_t) Context)
            {
                case (size_t) DPI_AWARENESS_CONTEXT_UNAWARE:
                {
                    Log.AtDebug().Write("DPI unaware.");
                    break;
                }

                case (size_t) DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
                {
                    Log.AtDebug().Write("System DPI unaware.");
                    break;
                }

                case (size_t) DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
                {
                    Log.AtDebug().Write("Per monitor DPI aware.");
                    break;
                }

                case (size_t) DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
                {
                    Log.AtDebug().Write("Per monitor DPI aware v2.");
                    break;
                }

                case (size_t) DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
                {
                    Log.AtDebug().Write("DPI unaware with improved quality of GDI-based content.");
                    break;
                }

                default:
                    Log.AtDebug().Write("Unknown DPI Awareness Context.");
            }
        }
    }

    {
        typedef UINT (WINAPI * GetDpiForWindow_t)(HWND hwnd);

        #pragma warning(disable: 4191) // 'type cast': unsafe conversion from 'FARPROC' to 'GetDPI::GetDpiForWindow_t'
        const auto GetDpiForWindow_ = (GetDpiForWindow_t) Module.GetFunctionAddress("GetDpiForWindow"); // Windows 10 or higher

        if (GetDpiForWindow_ != nullptr)
        {
            dpi = GetDpiForWindow_(hWnd);

            return S_OK;
        }
    }

    {
        ComPtr<ID2D1Factory2> D2DFactory;

        D2D1_FACTORY_OPTIONS const Options = { D2D1_DEBUG_LEVEL_NONE };

        HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, Options, D2DFactory.GetAddressOf());

        if (SUCCEEDED(hr))
        {
            FLOAT DPIX, DPIY;

            #pragma warning(disable: 4996) // 'ID2D1Factory::GetDesktopDpi': Deprecated.
            D2DFactory->GetDesktopDpi(&DPIX, &DPIY);
            #pragma warning(default: 4996)

            dpi = (UINT) DPIX;
        }
        else
            dpi = USER_DEFAULT_SCREEN_DPI;
    }

    return S_OK;
}

/// <summary>
/// Evaluates a foobar2000 Title Format script.
/// </summary>
HRESULT EvaluateTitleFormatScript(const std::wstring & script, pfc::string & result) noexcept
{
    service_ptr_t<titleformat_object> tfo;

    static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(tfo, pfc::utf8FromWide(script.c_str()));

    if (!tfo.is_valid())
        return S_FALSE;

    custom_titleformat_hook_t Hook;

    tfo->run(&Hook, result, nullptr);

    return S_OK;
}

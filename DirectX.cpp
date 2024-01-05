
/** $VER: DirectX.cpp (2024.01.01) P. Stuer **/

#include "DirectX.h"

#include "SafeModuleHandle.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
DirectX::DirectX()
{
    CreateDeviceIndependentResources();
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT DirectX::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    if (_Direct2D == nullptr)
        hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &_Direct2D);

    if ((_DirectWrite == nullptr) && SUCCEEDED(hr))
        hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(_DirectWrite), reinterpret_cast<IUnknown **>(&_DirectWrite));

    return hr;
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
        _Direct2D->GetDesktopDpi(&DPIX, &DPIY);
        #pragma warning(default: 4996)

        dpi = (UINT) DPIX;
    }

    return S_OK;
}

DirectX _DirectX;

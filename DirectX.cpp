
/** $VER: DirectX.cpp (2023.12.31) P. Stuer **/

#include "DirectX.h"

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
/// Releases the device independent resources.
/// </summary>
void DirectX::ReleaseDeviceIndependentResources()
{
//  _DirectWrite.Release();
//  _Direct2D.Release();
}

HRESULT DirectX::GetDPI(HWND hWnd, UINT & dpi) const
{
    if (::IsWindows10OrGreater())
        dpi = ::GetDpiForWindow(hWnd);
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

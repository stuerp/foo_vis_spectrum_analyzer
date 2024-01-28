
/** $VER: Direct3D.cpp (2024.01.28) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Direct3D.h"

#include "COMException.h"

#pragma comment(lib, "d3d11")

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT Direct3D::Initialize()
{
#ifdef _DEBUG
    const DWORD Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG; // Enables interoperability with Direct2D.
 #else
    const DWORD Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Enables interoperability with Direct2D.
#endif

    HRESULT hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, Flags, nullptr, 0, D3D11_SDK_VERSION, &Device, nullptr, nullptr);

    if (!SUCCEEDED(hr))
        throw COMException(hr, L"Unable to create Direct3D device.");

    return hr;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void Direct3D::Terminate()
{
    Device.Release();
}

/// <summary>
/// Gets the DXGI interface.
/// </summary>
HRESULT Direct3D::GetDXGIDevice(IDXGIDevice ** device) const noexcept
{
    return Device.QueryInterface(device);
}

Direct3D _Direct3D;

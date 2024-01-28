
/** $VER: DXGI.cpp (2024.01.28) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Direct3D.h"

#include "COMException.h"

#pragma comment(lib, "dxgi")

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT DXGI::Initialize()
{
#ifdef _DEBUG
    const DWORD Flags = DXGI_CREATE_FACTORY_DEBUG;
#else
    const DWORD Flags = 0;
#endif

    HRESULT hr = ::CreateDXGIFactory2(Flags, __uuidof(Factory), (void **) &Factory);

    if (!SUCCEEDED(hr))
        throw COMException(hr, L"Unable to create DXGI factory.");
}

/// <summary>
/// Terminates this instance.
/// </summary>
void DXGI::Terminate()
{
    Factory.Release();
}

/// <summary>
/// Creates a custom swap chain.
/// </summary>
HRESULT DXGI::CreateSwapChain(IDXGIDevice * dxgiDevice, UINT width, UINT height, IDXGISwapChain1 ** swapChain) const noexcept
{
    DXGI_SWAP_CHAIN_DESC1 scd = {};

    scd.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;      // Best performance and compatibility
    scd.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    scd.BufferCount      = 2;
    scd.SampleDesc.Count = 1;                               // Number of multisamples per pixel (Multisampling disabled).
    scd.AlphaMode        = DXGI_ALPHA_MODE_PREMULTIPLIED;   // Enable transparency

    // Set the size of the buffers. DXGI does not know what the swap chain will be used for and will not query the window for its client area size.
    scd.Width  = width;
    scd.Height = height;

    return Factory->CreateSwapChainForComposition(dxgiDevice, &scd, nullptr, swapChain);
}

DXGI _DXGI;

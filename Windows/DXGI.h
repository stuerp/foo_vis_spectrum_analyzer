
/** $VER: DXGI.h (2026.09.26) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dxgi1_6.h>

#include <wrl/client.h>

#include <Win32Exception.h>

class DXGIFactory
{
public:
    DXGIFactory(const DXGIFactory & ) = delete;
    DXGIFactory & operator=(const DXGIFactory &) = delete;

    [[nodiscard]]
    static IDXGIFactory7 * Get()
    {
        return Instance()._Factory.Get();
    }

    static void Shutdown()
    {
        Instance()._Factory.Reset();
    }

private:
    DXGIFactory()
    {
        #ifdef _DEBUG
            constexpr UINT Flags = DXGI_CREATE_FACTORY_DEBUG;
        #else
            constexpr UINT Flags = 0;
        #endif

        HRESULT hr = ::CreateDXGIFactory2(Flags, IID_PPV_ARGS(_Factory.ReleaseAndGetAddressOf()));

        if (FAILED(hr))
            throw msc::win32_exception("Unable to create Direct2D factory.", (DWORD) hr);
    }

    static DXGIFactory & Instance()
    {
        static DXGIFactory Instance;

        return Instance;
    }

private:
    ComPtr<IDXGIFactory7> _Factory;
};

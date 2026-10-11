
/** $VER: DXGI.h (2026.10.04) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dxgi1_6.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DXGIFactory final
{
    typedef IDXGIFactory2 Interface;

public:
    DXGIFactory(const DXGIFactory &) = delete;
    DXGIFactory & operator=(const DXGIFactory &) = delete;

    [[nodiscard]]
    static ComPtr<Interface> Get() noexcept
    {
        return Instance()._Factory.Get();
    }

    static HRESULT Startup() noexcept
    {
        return Instance().Initialize();
    }

    static void Shutdown() noexcept
    {
        Instance().Terminate();
    }

private:
    DXGIFactory() noexcept = default;

    static DXGIFactory & Instance() noexcept
    {
        static DXGIFactory _Instance;

        return _Instance;
    }

    HRESULT Initialize() noexcept;
    void Terminate() noexcept;

private:
    static ComPtr<Interface> _Factory;
    static int64_t _ReferenceCount;

    std::mutex _Mutex;
};

class DXGI
{
public:
    static HRESULT GetRefreshRate(IDXGIDevice1 * dxgiDevice, double & refreshRate) noexcept;
};

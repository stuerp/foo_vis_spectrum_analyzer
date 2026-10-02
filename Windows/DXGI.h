
/** $VER: DXGI.h (2026.10.01) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dxgi1_6.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DXGIFactory final
{
public:
    DXGIFactory(const DXGIFactory &) = delete;
    DXGIFactory& operator=(const DXGIFactory &) = delete;

    [[nodiscard]]
    static ComPtr<IDXGIFactory7> Get() noexcept;

    static HRESULT Startup() noexcept;
    static void Shutdown() noexcept;

private:
    DXGIFactory() noexcept = default;

    static DXGIFactory & Instance() noexcept;

    HRESULT Initialize() noexcept;
    void Terminate() noexcept;

private:
    static ComPtr<IDXGIFactory7> _Factory;
    static int64_t _ReferenceCount;

    std::mutex _Mutex;
};

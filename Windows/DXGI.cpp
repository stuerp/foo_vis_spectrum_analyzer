
/** $VER: DXGI.cpp (2026.10.01) P. Stuer **/

#include "pch.h"

#include "DXGI.h"

#pragma comment(lib, "dxgi")

#pragma hdrstop

ComPtr<IDXGIFactory7> DXGIFactory::_Factory;
int64_t DXGIFactory::_ReferenceCount = 0;

ComPtr<IDXGIFactory7> DXGIFactory::Get() noexcept
{
    return Instance()._Factory.Get();
}

HRESULT DXGIFactory::Startup() noexcept
{
    return Instance().Initialize();
}

void DXGIFactory::Shutdown() noexcept
{
    Instance().Terminate();
}

DXGIFactory & DXGIFactory::Instance() noexcept
{
    static DXGIFactory _Instance;

    return _Instance;
}

HRESULT DXGIFactory::Initialize() noexcept
{
    std::lock_guard Lock(_Mutex);

    ++_ReferenceCount;

    if (_Factory)
        return S_OK;

    {
        #ifdef _DEBUG
            constexpr UINT Flags = DXGI_CREATE_FACTORY_DEBUG;
        #else
            constexpr UINT Flags = 0;
        #endif

        return ::CreateDXGIFactory2(Flags, IID_PPV_ARGS(_Factory.GetAddressOf()));
    }
}

void DXGIFactory::Terminate() noexcept
{
    std::lock_guard Lock(_Mutex);

    --_ReferenceCount;

    if (_ReferenceCount == 0)
        _Factory.Reset();
}

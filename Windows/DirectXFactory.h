
/** $VER: Direct2D.h (2026.09.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

template <typename Derived, typename FactoryInterface>
class DirectXFactory
{
public:
    DirectXFactory(const DirectXFactory &) = delete;
    DirectXFactory & operator=(const DirectXFactory &) = delete;

    [[nodiscard]]
    static ComPtr<FactoryInterface> Get()
    {
        auto & Instance = GetInstance();

        std::scoped_lock Lock(Instance._Mutex);

        if (Instance._Factory == nullptr)
            Instance.CreateFactory();

        return Instance._Factory; // Calls AddRef()
    }

    static void Shutdown()
    {
        auto & Instance = GetInstance();

        std::scoped_lock Lock(Instance._Mutex);

        Instance._Factory.Reset();
    }

protected:
    DirectXFactory() = default;
    ~DirectXFactory() = default;

    ComPtr<FactoryInterface> _Factory;

private:
    static Derived & GetInstance()
    {
        static Derived Instance;

        return Instance;
    }

    virtual void CreateFactory() { };

private:
    std::mutex _Mutex;
};

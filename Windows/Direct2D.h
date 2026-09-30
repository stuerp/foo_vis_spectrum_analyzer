
/** $VER: Direct2D.h (2026.09.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <d2d1_2.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <Win32Exception.h>

#include "DirectXFactory.h"

class Direct2DFactory final : public DirectXFactory<Direct2DFactory, ID2D1Factory1>
{
    friend class DirectXFactory<Direct2DFactory, ID2D1Factory1>;

public:
    Direct2DFactory(const Direct2DFactory &) = delete;
    Direct2DFactory & operator=(const Direct2DFactory &) = delete;

private:
    Direct2DFactory() = default;
    ~Direct2DFactory() = default;

    void CreateFactory()
    {
        ComPtr<ID2D1Factory1> Factory;

        {
            #ifdef _DEBUG
                constexpr D2D1_FACTORY_OPTIONS Options = { D2D1_DEBUG_LEVEL_INFORMATION };
            #else
                constexpr D2D1_FACTORY_OPTIONS Options = { D2D1_DEBUG_LEVEL_NONE };
            #endif

            const HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, Options, Factory.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                throw msc::win32_exception("Unable to create Direct2D factory.", static_cast<DWORD>(hr));
        }

        _Factory = std::move(Factory);
    }
};
/*
class Direct2DFactory final
{
public:
    Direct2DFactory(const Direct2DFactory &) = delete;
    Direct2DFactory & operator=(const Direct2DFactory &) = delete;

    [[nodiscard]]
    static ComPtr<ID2D1Factory1> Get()
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

private:
    Direct2DFactory() = default;

    static Direct2DFactory & GetInstance()
    {
        static Direct2DFactory Instance;

        return Instance;
    }

    void CreateFactory()
    {
        #ifdef _DEBUG
            constexpr D2D1_FACTORY_OPTIONS Options = { D2D1_DEBUG_LEVEL_INFORMATION };
        #else
            constexpr D2D1_FACTORY_OPTIONS Options = { D2D1_DEBUG_LEVEL_NONE };
        #endif

        ComPtr<ID2D1Factory1> Factory;

        const HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, Options, Factory.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            throw msc::win32_exception("Unable to create Direct2D factory.", static_cast<DWORD>(hr));

        _Factory = std::move(Factory);
    }

private:
    ComPtr<ID2D1Factory1> _Factory;
    std::mutex _Mutex;
};
*/
class Direct2D
{
public:
    static HRESULT GetRefreshRate(IDXGIDevice1 * dxgiDevice, double & refreshRate) noexcept;

    static HRESULT Load(const WCHAR * resourceName, const WCHAR * resourceType, IWICBitmapSource ** source) noexcept;
    static HRESULT Load(const WCHAR * uri, IWICBitmapSource ** source) noexcept;

    static HRESULT CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, IWICBitmapScaler ** scaler) noexcept;

    static HRESULT CreateBitmap(IWICBitmapSource * source, ID2D1DeviceContext * deviceContext, ID2D1Bitmap ** bitmap) noexcept;

    static HRESULT CreateGradientBrush(ID2D1DeviceContext * deviceContext, const std::vector<D2D1_GRADIENT_STOP> & gradientStops, const D2D1_SIZE_F & size, bool isHorizontal, ID2D1LinearGradientBrush ** gradientBrush) noexcept;
    static HRESULT CreateRadialGradientBrush(ID2D1DeviceContext * deviceContext, const std::vector<D2D1_GRADIENT_STOP> & gradientStops, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, ID2D1RadialGradientBrush ** gradientBrush) noexcept;

private:
    static HRESULT GetResource(const WCHAR * resourceName, const WCHAR * resourceType, void ** resourceData, DWORD * resourceSize) noexcept;
};

/// <summary>
/// A more sane way of representing a rectangle
/// </summary>
struct rect_t
{
    rect_t & operator =(const D2D1_RECT_F & other) noexcept
    {
        *this = other;

        return *this;
    }

    operator D2D1_RECT_F () const noexcept
    {
        return { x1, y1, x2, y2 };
    }

    D2D1_SIZE_F Size() const noexcept { return { std::abs(x1 - x2), std::abs(y1 - y2) }; }
    FLOAT Width() const noexcept { return std::abs(x2 - x1); }
    FLOAT Height() const noexcept { return std::abs(y2 - y1); }

    FLOAT x1;
    FLOAT y1;
    FLOAT x2;
    FLOAT y2;
};

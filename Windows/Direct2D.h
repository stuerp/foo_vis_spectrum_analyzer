
/** $VER: Direct2D.h (2026.10.04) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <d2d1_2.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <mutex>

class Direct2DFactory final
{
    typedef ID2D1Factory2 Interface;

public:
    Direct2DFactory(const Direct2DFactory &) = delete;
    Direct2DFactory & operator=(const Direct2DFactory &) = delete;

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
    Direct2DFactory() noexcept = default;

    static Direct2DFactory & Instance() noexcept
    {
        static Direct2DFactory _Instance;

        return _Instance;
    }

    HRESULT Initialize() noexcept;
    void Terminate() noexcept;

private:
    static ComPtr<Interface> _Factory;
    static int64_t _ReferenceCount;

    std::mutex _Mutex;
};

class Direct2D
{
public:
    static HRESULT Load(const WCHAR * resourceName, const WCHAR * resourceType, IWICBitmapSource ** source) noexcept;
    static HRESULT Load(const WCHAR * uri, IWICBitmapSource ** source) noexcept;

    static HRESULT CreateScaler(IWICBitmapSource * source, UINT width, UINT height, UINT maxWidth, UINT maxHeight, IWICBitmapScaler ** scaler) noexcept;

    static HRESULT CreateBitmap(IWICBitmapSource * source, ID2D1DeviceContext * deviceContext, ID2D1Bitmap ** bitmap) noexcept;

    static HRESULT CreateGradientBrush(ID2D1DeviceContext * deviceContext, const std::vector<D2D1_GRADIENT_STOP> & gradientStops, const D2D1_SIZE_F & size, bool isHorizontal, ID2D1LinearGradientBrush ** gradientBrush) noexcept;
    static HRESULT CreateRadialGradientBrush(ID2D1DeviceContext * deviceContext, const std::vector<D2D1_GRADIENT_STOP> & gradientStops, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, ID2D1RadialGradientBrush ** gradientBrush) noexcept;

private:
    Direct2D() = default;

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

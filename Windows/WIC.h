
/** $VER: WIC.h (2026.09.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <wincodec.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <Win32Exception.h>

#include "DirectXFactory.h"

class WICFactory final : public DirectXFactory<WICFactory, IWICImagingFactory3>
{
    friend class DirectXFactory<WICFactory, IWICImagingFactory3>;

public:
    WICFactory(const WICFactory &) = delete;
    WICFactory & operator=(const WICFactory &) = delete;

private:
    WICFactory() = default;
    ~WICFactory() = default;

    void CreateFactory()
    {
        ComPtr<IWICImagingFactory3> Factory;

        {
            HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(Factory.ReleaseAndGetAddressOf()));

            if (FAILED(hr))
                throw msc::win32_exception("Unable to create WIC factory.", (DWORD) hr);
        }

        _Factory = std::move(Factory);
    }
};
/*
class WICFactory
{
public:
    WICFactory(const WICFactory &) = delete;
    WICFactory & operator=(const WICFactory &) = delete;

    [[nodiscard]]
    static IWICImagingFactory3 * Get()
    {
        return Instance()._Factory.Get();
    }

    static void Shutdown()
    {
        Instance()._Factory.Reset();
    }

private:
    WICFactory()
    {
        HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(_Factory.ReleaseAndGetAddressOf()));

        if (FAILED(hr))
            throw msc::win32_exception("Unable to create WIC factory.", (DWORD) hr);
    }

    static WICFactory & Instance()
    {
        static WICFactory Instance;

        return Instance;
    }

private:
    ComPtr<IWICImagingFactory3> _Factory;
};
*/
#include <string>

class WIC
{
public:
    static HRESULT Load(const uint8_t * data, size_t size, IWICBitmapFrameDecode ** frame) noexcept;
    static HRESULT Load(const std::wstring & filePath, IWICBitmapFrameDecode ** frame) noexcept;

    static HRESULT GetFormatConverter(IWICBitmapFrameDecode * frame, IWICFormatConverter ** formatConverter) noexcept;

    static HRESULT CreateBitmapFromSource(IWICBitmapSource * bitmapSource, WICBitmapCreateCacheOption option, IWICBitmap ** bitmap)
    {
        return WICFactory::Get()->CreateBitmapFromSource(bitmapSource, option, bitmap);
    }

    static HRESULT GetBitsPerPixel(const WICPixelFormatGUID & pixelFormat, UINT & BitsPerPixel) noexcept;
};

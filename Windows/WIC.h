
/** $VER: WIC.h (2026.09.26) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <wincodec.h>

#include <string>

#include <Win32Exception.h>

class WICFactory
{
public:
    WICFactory(const WICFactory & ) = delete;
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

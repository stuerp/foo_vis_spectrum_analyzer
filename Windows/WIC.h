
/** $VER: WIC.h (2026.10.01) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <wincodec.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <mutex>

class WICFactory final
{
public:
    WICFactory(const WICFactory &) = delete;
    WICFactory& operator=(const WICFactory &) = delete;

    [[nodiscard]]
    static ComPtr<IWICImagingFactory3> Get() noexcept;

    static HRESULT Startup() noexcept;
    static void Shutdown() noexcept;

private:
    WICFactory() noexcept = default;

    static WICFactory & Instance() noexcept;

    HRESULT Initialize() noexcept;
    void Terminate() noexcept;

private:
    static ComPtr<IWICImagingFactory3> _Factory;
    static int64_t _ReferenceCount;

    std::mutex _Mutex;
};

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

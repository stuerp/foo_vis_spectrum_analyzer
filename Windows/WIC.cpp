
/** $VER: WIC.cpp (2026.10.04) P. Stuer **/

#include "pch.h"

#include "WIC.h"

#pragma comment(lib, "windowscodecs")

ComPtr<WICFactory::Interface> WICFactory::_Factory;
int64_t WICFactory::_ReferenceCount = 0;

ComPtr<WICFactory::Interface> WICFactory::Get() noexcept
{
    return Instance()._Factory.Get();
}

HRESULT WICFactory::Startup() noexcept
{
    return Instance().Initialize();
}

void WICFactory::Shutdown() noexcept
{
    Instance().Terminate();
}

WICFactory & WICFactory::Instance() noexcept
{
    static WICFactory _Instance;

    return _Instance;
}

HRESULT WICFactory::Initialize() noexcept
{
    std::lock_guard Lock(_Mutex);

    ++_ReferenceCount;

    if (_Factory)
        return S_OK;

    return ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_Factory));
}

void WICFactory::Terminate() noexcept
{
    std::lock_guard Lock(_Mutex);

    --_ReferenceCount;

    if (_ReferenceCount == 0)
        _Factory.Reset();
}

/// <summary>
/// Creates a WIC bitmap frame from raw image data.
/// </summary>
HRESULT WIC::Load(const uint8_t * data, size_t size, IWICBitmapFrameDecode ** frame) noexcept
{
    if ((data == nullptr) || (size == 0))
        return E_FAIL;

    ComPtr<IWICStream> Stream;

    HRESULT hr = WICFactory::Get()->CreateStream(Stream.GetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = Stream->InitializeFromMemory((BYTE *) data, (DWORD) size);

    if (FAILED(hr))
        return hr;

    ComPtr<IWICBitmapDecoder> Decoder;

    hr = WICFactory::Get()->CreateDecoderFromStream(Stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, Decoder.GetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = Decoder->GetFrame(0, frame);

    return hr;
}

/// <summary>
/// Creates a WIC bitmap frame from a file.
/// </summary>
HRESULT WIC::Load(const std::wstring & filePath, IWICBitmapFrameDecode ** frame) noexcept
{
    if (filePath.empty())
        return E_FAIL;

    ComPtr<IWICBitmapDecoder> Decoder;

    HRESULT hr = WICFactory::Get()->CreateDecoderFromFilename(filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, Decoder.GetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = Decoder->GetFrame(0, frame);

    return hr;
}

/// <summary>
/// Gets the number of bits per pixel for the specified pixel format.
/// </summary>
HRESULT WIC::GetBitsPerPixel(const WICPixelFormatGUID & pixelFormat, UINT & bitsPerPixel) noexcept
{
    ComPtr<IWICComponentInfo> ComponentInfo;

    HRESULT hr = WICFactory::Get()->CreateComponentInfo(pixelFormat, ComponentInfo.GetAddressOf());

    if (FAILED(hr))
        return hr;

    ComPtr<IWICPixelFormatInfo> PixelFormatInfo;

    hr = ComponentInfo.As(&PixelFormatInfo);

    if (FAILED(hr))
        return hr;

    hr = PixelFormatInfo->GetBitsPerPixel(&bitsPerPixel);

    return hr;
}

/// <summary>
/// Creates a format converter to convert a WIC frame to the specfied format.
/// </summary>
HRESULT WIC::GetFormatConverter(IWICBitmapFrameDecode * frame, IWICFormatConverter ** formatConverter) noexcept
{
    HRESULT hr = WICFactory::Get()->CreateFormatConverter(formatConverter);

    if (FAILED(hr))
        return hr;

    // Convert the format of the frame to 32bppPBGRA.
    hr = (*formatConverter)->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);

    return hr;
}

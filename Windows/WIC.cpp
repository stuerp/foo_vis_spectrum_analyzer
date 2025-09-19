
/** $VER: WIC.cpp (2024.01.29) P. Stuer **/

#include "pch.h"
#include "WIC.h"

#include <libmsc.h>

#pragma comment(lib, "windowscodecs")

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT WIC::Initialize()
{
    HRESULT hr = ::CoInitialize(nullptr);

    hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Factory));

    if (!SUCCEEDED(hr))
        throw msc::win32_exception("Unable to create WIC factory.", (DWORD) hr);

    return hr;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void WIC::Terminate()
{
    Factory.Release();
}

/// <summary>
/// Creates a WIC bitmap frame from raw image data.
/// </summary>
HRESULT WIC::Load(const uint8_t * data, size_t size, IWICBitmapFrameDecode ** frame) const noexcept
{
    if ((data == nullptr) || (size == 0))
        return E_FAIL;

    CComPtr<IWICStream> Stream;

    HRESULT hr = Factory->CreateStream(&Stream);

    if (SUCCEEDED(hr))
        hr = Stream->InitializeFromMemory((BYTE *) data, (DWORD) size);

    CComPtr<IWICBitmapDecoder> Decoder;

    if (SUCCEEDED(hr))
        hr = Factory->CreateDecoderFromStream(Stream, nullptr, WICDecodeMetadataCacheOnDemand, &Decoder);

    if (SUCCEEDED(hr))
        hr = Decoder->GetFrame(0, frame);

    return hr;
}

/// <summary>
/// Creates a WIC bitmap frame from a file.
/// </summary>
HRESULT WIC::Load(const std::wstring & filePath, IWICBitmapFrameDecode ** frame) const noexcept
{
    if (filePath.empty())
        return E_FAIL;

    CComPtr<IWICBitmapDecoder> Decoder;

    HRESULT hr = Factory->CreateDecoderFromFilename(filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &Decoder);

    if (SUCCEEDED(hr))
        hr = Decoder->GetFrame(0, frame);

    return hr;
}

/// <summary>
/// Gets the number of bits per pixel for the specified pixel format.
/// </summary>
HRESULT WIC::GetBitsPerPixel(const WICPixelFormatGUID & pixelFormat, UINT & bitsPerPixel) const noexcept
{
    CComPtr<IWICComponentInfo> ComponentInfo;

    HRESULT hr = Factory->CreateComponentInfo(pixelFormat, &ComponentInfo);

    CComPtr<IWICPixelFormatInfo> PixelFormatInfo;

    if (SUCCEEDED(hr))
        hr = ComponentInfo->QueryInterface(__uuidof(IWICPixelFormatInfo), (void **) &PixelFormatInfo);

    if (SUCCEEDED(hr))
        hr = PixelFormatInfo->GetBitsPerPixel(&bitsPerPixel);

    return hr;
}

/// <summary>
/// Creates a format converter to convert a WIC frame to the specfied format.
/// </summary>
HRESULT WIC::GetFormatConverter(IWICBitmapFrameDecode * frame, IWICFormatConverter ** formatConverter) const noexcept
{
    // Convert the format of the frame to 32bppPBGRA.
    HRESULT hr = Factory->CreateFormatConverter(formatConverter);

    if (SUCCEEDED(hr))
        hr = (*formatConverter)->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);

    return hr;
}

WIC _WIC;

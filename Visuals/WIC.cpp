
/** $VER: WIC.cpp (2024.01.07) P. Stuer **/

#include "WIC.h"

#include <map>
#include <algorithm>

#include "Support.h"

#include "DominantColors.h"
#include "Pixels.h"

#pragma comment(lib, "windowscodecs")

/// <summary>
/// Initializes a new instance.
/// </summary>
WIC::WIC()
{
    HRESULT hr = ::CoInitialize(nullptr);

    hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_WICFactory));
}

/// <summary>
/// Creates a WIC format converter from raw image data.
/// </summary>
HRESULT WIC::Load(const uint8_t * data, size_t size, CComPtr<IWICBitmapFrameDecode> & frame) const noexcept
{
    if ((data == nullptr) || (size == 0))
        return E_FAIL;

    CComPtr<IWICStream> Stream;

    HRESULT hr = _WICFactory->CreateStream(&Stream);

    if (SUCCEEDED(hr))
        hr = Stream->InitializeFromMemory((BYTE *) data, (DWORD) size);

    CComPtr<IWICBitmapDecoder> Decoder;

    if (SUCCEEDED(hr))
        hr = _WICFactory->CreateDecoderFromStream(Stream, nullptr, WICDecodeMetadataCacheOnLoad, &Decoder);

    if (SUCCEEDED(hr))
        hr = Decoder->GetFrame(0, &frame);

    return hr;
}

/// <summary>
/// Creates a histogram for the specified frame.
/// </summary>
/// <comment>This is a very naive and inefficient implementation.</comment>
HRESULT WIC::GetHistogram(CComPtr<IWICBitmapFrameDecode> frame, std::vector<std::pair<uint32_t, uint32_t>> & bins) const noexcept
{
    typedef std::pair<uint32_t, uint32_t> Bin;

    // Create the bitmap from the image frame.
    CComPtr<IWICBitmap> Bitmap;

    HRESULT hr = _WICFactory->CreateBitmapFromSource(frame, WICBitmapCacheOnDemand, &Bitmap);

    CComPtr<IWICBitmapLock> Lock;
    UINT Width = 0, Height = 0;

    if (SUCCEEDED(hr))
    {
        // Lock the complete bitmap.
        Bitmap->GetSize(&Width, &Height);

        WICRect LockRect = { 0, 0, (INT) Width, (INT) Height };

        hr = Bitmap->Lock(&LockRect, WICBitmapLockRead, &Lock);
    }

    UINT Size = 0;
    BYTE * Data = nullptr;

        // Retrieve a pointer to the pixel data.
    if (SUCCEEDED(hr))
        hr = Lock->GetDataPointer(&Size, &Data);

    WICPixelFormatGUID PixelFormat;

    if (SUCCEEDED(hr))
        hr = Lock->GetPixelFormat(&PixelFormat);

    UINT BitsPerPixel = 0;

    if (SUCCEEDED(hr))
        hr = GetBitsPerPixel(PixelFormat, BitsPerPixel);

    UINT Stride = 0;

    if (SUCCEEDED(hr))
        hr = Lock->GetStride(&Stride);

    #pragma warning(disable: 6011 6385)
    if (SUCCEEDED(hr) && (BitsPerPixel == 24 || BitsPerPixel == 32))
    {
        UINT BytesPerPixel = BitsPerPixel >> 3;

        if (SUCCEEDED(hr))
        {
            std::map<uint32_t, uint32_t> Map;

            for (UINT y = 0; y < Height; y++)
            {
                const BYTE * Line = Data;

                for (UINT x = 0; x < Width; x += BytesPerPixel)
                {
                    uint32_t Color = (uint32_t) Line[2];

                    Color <<= 8;
                    Color |= (uint32_t) Line[1];

                    Color <<= 8;
                    Color |= (uint32_t) Line[0];

                    Line += BytesPerPixel;

                    if (Map.find(Color) == Map.end())
                        Map[Color] = 1;
                    else
                        Map[Color]++;
                }

                Data += Stride;
            }

            bins.reserve(Map.size());
   
            for (const auto & it : Map)
                bins.push_back(it);
   
            std::sort(bins.begin(), bins.end(), [](Bin a, Bin b) { return a.second > b.second; });
        }
    }
    #pragma warning(default: 6011 6385)

    return hr;
}

/// <summary>
/// Gets the number of bits per pixel for the specified pixel format.
/// </summary>
HRESULT WIC::GetBitsPerPixel(const WICPixelFormatGUID & pixelFormat, UINT & bitsPerPixel) const noexcept
{
    CComPtr<IWICComponentInfo> ComponentInfo;

    HRESULT hr = _WICFactory->CreateComponentInfo(pixelFormat, &ComponentInfo);

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
HRESULT WIC::GetFormatConverter(CComPtr<IWICBitmapFrameDecode> frame, CComPtr<IWICFormatConverter> & formatConverter) const noexcept
{
    // Convert the format of the frame to 32bppPBGRA.
    HRESULT hr = _WICFactory->CreateFormatConverter(&formatConverter);

    if (SUCCEEDED(hr))
        hr = formatConverter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);

    return hr;
}

WIC _WIC;


/** $VER: WIC.cpp (2024.01.05) P. Stuer **/

#include "WIC.h"

#include <map>
#include <algorithm>

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
CComPtr<IWICFormatConverter> WIC::Load(const uint8_t * data, size_t size) const
{
    if ((data == nullptr) || (size == 0))
        return nullptr;

    CComPtr<IWICStream> Stream;

    HRESULT hr = _WICFactory->CreateStream(&Stream);

    if (SUCCEEDED(hr))
        hr = Stream->InitializeFromMemory((BYTE *) data, (DWORD) size);

    CComPtr<IWICBitmapDecoder> Decoder;

    if (SUCCEEDED(hr))
        hr = _WICFactory->CreateDecoderFromStream(Stream, nullptr, WICDecodeMetadataCacheOnLoad, &Decoder);

    CComPtr<IWICBitmapFrameDecode> Frame;

    if (SUCCEEDED(hr))
        hr = Decoder->GetFrame(0, &Frame);

    {
        CComPtr<IWICBitmap> Bitmap;

        // Create the bitmap from the image frame.
        if (SUCCEEDED(hr))
            hr = _WICFactory->CreateBitmapFromSource(Frame, WICBitmapCacheOnDemand, &Bitmap);

        CComPtr<IWICBitmapLock> Lock;

        UINT Width, Height;

        Bitmap->GetSize(&Width, &Height);

        WICRect LockRect = { 0, 0, (INT) Width, (INT) Height };

        if (SUCCEEDED(hr))
            hr = Bitmap->Lock(&LockRect, WICBitmapLockRead, &Lock);

        if (SUCCEEDED(hr))
        {
            UINT Size = 0;
            BYTE * Data = nullptr;

            // Retrieve a pointer to the pixel data.
            if (SUCCEEDED(hr))
                hr = Lock->GetDataPointer(&Size, &Data);

            if (Data != nullptr)
            {
                WICPixelFormatGUID PixelFormat;

                Lock->GetPixelFormat(&PixelFormat);

                UINT Stride;

                Lock->GetStride(&Stride);

                std::map<DWORD, uint32_t> Map;

                for (UINT y = 0; y < Height; ++y)
                {
                    for (UINT x = 0; x < Width; x += 3)
                    {
                        DWORD Color = ((((DWORD) Data[x] << 8) + (DWORD) Data[x + 1]) << 8) + (DWORD) Data[x + 2];

                        if (Map.find(Color) == Map.end())
                            Map[Color] = 1;
                        else
                            Map[Color]++;
                    }

                    Data += Stride;
                }

                typedef std::pair<DWORD, uint32_t> Bin;

                std::vector<Bin> v;
   
                for (auto & it : Map)
                    v.push_back(it);
   
                std::sort(v.begin(), v.end(), [](Bin a, Bin b) { return a.second > b.second; });
            }
        }
    }

   // Format convert the frame to 32bppPBGRA
    CComPtr<IWICFormatConverter> FormatConverter;

    if (SUCCEEDED(hr))
       hr = _WICFactory->CreateFormatConverter(&FormatConverter);

   if (SUCCEEDED(hr))
       hr = FormatConverter->Initialize(Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);

    return FormatConverter;
}

WIC _WIC;

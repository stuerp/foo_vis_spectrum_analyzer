
/** $VER: WIC.h (2024.01.07) P. Stuer **/

#pragma once

#include "framework.h"

#include "DominantColors.h"

class WIC
{
public:
    WIC();

    HRESULT Load(const uint8_t * data, size_t size, CComPtr<IWICBitmapFrameDecode> & frame) const noexcept;

    HRESULT GetHistogram(CComPtr<IWICBitmapFrameDecode> frame, std::vector<std::pair<uint32_t, uint32_t>> & bins) const noexcept;
    HRESULT GetDominantColors(CComPtr<IWICBitmapFrameDecode> frame, size_t count, std::vector<uint32_t> & colors) const noexcept;
    HRESULT GetFormatConverter(CComPtr<IWICBitmapFrameDecode> frame, CComPtr<IWICFormatConverter> & formatConverter) const noexcept;

    HRESULT CreateBitmapFromSource(IWICBitmapSource * bitmapSource, WICBitmapCreateCacheOption option, IWICBitmap ** bitmap)
    {
        return _WICFactory->CreateBitmapFromSource(bitmapSource, option, bitmap);
    }

    HRESULT GetBitsPerPixel(const WICPixelFormatGUID & pixelFormat, UINT & BitsPerPixel) const noexcept;

public:
    CComPtr<IWICImagingFactory> _WICFactory;
};

extern WIC _WIC;

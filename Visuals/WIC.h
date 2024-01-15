
/** $VER: WIC.h (2024.01.08) P. Stuer **/

#pragma once

#include "framework.h"

class WIC
{
public:
    WIC();

    HRESULT Load(const uint8_t * data, size_t size, CComPtr<IWICBitmapFrameDecode> & frame) const noexcept;

    HRESULT GetFormatConverter(CComPtr<IWICBitmapFrameDecode> frame, CComPtr<IWICFormatConverter> & formatConverter) const noexcept;

    HRESULT CreateBitmapFromSource(IWICBitmapSource * bitmapSource, WICBitmapCreateCacheOption option, IWICBitmap ** bitmap)
    {
        return Factory->CreateBitmapFromSource(bitmapSource, option, bitmap);
    }

    HRESULT GetBitsPerPixel(const WICPixelFormatGUID & pixelFormat, UINT & BitsPerPixel) const noexcept;

public:
    CComPtr<IWICImagingFactory> Factory;
};

extern WIC _WIC;

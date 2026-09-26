
/** $VER: Raster.cpp (2024.03.09) P. Stuer **/

#include "pch.h"

#include "Raster.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance from a WIC bitmap source.
/// </summary>
HRESULT raster_t::Initialize(IWICBitmapSource * bitmapSource) noexcept
{
    // Create the bitmap from the image frame.
    HRESULT hr = WICFactory::Get()->CreateBitmapFromSource(bitmapSource, WICBitmapCacheOnDemand, &_Bitmap);

    if (FAILED(hr))
        return hr;

    hr = _Bitmap->GetSize(&Width, &Height);

    if (FAILED(hr))
        return hr;

    // Lock the complete bitmap.
    {
        WICRect LockRect = { 0, 0, (INT) Width, (INT) Height };

        hr = _Bitmap->Lock(&LockRect, WICBitmapLockRead, _Lock.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    hr = _Lock->GetDataPointer(&Size, &Data);

    if (FAILED(hr))
        return hr;

    hr = _Lock->GetStride(&Stride);

    if (FAILED(hr))
        return hr;

    hr = _Lock->GetPixelFormat(&PixelFormat);

    if (FAILED(hr))
        return hr;

    hr = WIC::GetBitsPerPixel(PixelFormat, BitsPerPixel);

    return hr;
}

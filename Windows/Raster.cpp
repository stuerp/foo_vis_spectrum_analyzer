
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
    HRESULT hr = _WIC.CreateBitmapFromSource(bitmapSource, WICBitmapCacheOnDemand, &_Bitmap);

    if (SUCCEEDED(hr))
        hr = _Bitmap->GetSize(&Width, &Height);

    // Lock the complete bitmap.
    if (SUCCEEDED(hr))
    {
        WICRect LockRect = { 0, 0, (INT) Width, (INT) Height };

        hr = _Bitmap->Lock(&LockRect, WICBitmapLockRead, &_Lock);
    }

    if (SUCCEEDED(hr))
        hr = _Lock->GetDataPointer(&Size, &Data);

    if (SUCCEEDED(hr))
        hr = _Lock->GetStride(&Stride);

    if (SUCCEEDED(hr))
        hr = _Lock->GetPixelFormat(&PixelFormat);

    if (SUCCEEDED(hr))
        hr = _WIC.GetBitsPerPixel(PixelFormat, BitsPerPixel);

    return hr;
}

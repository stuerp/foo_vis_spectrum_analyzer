
/** $VER: Raster.cpp (2024.03.09) P. Stuer **/

#include "Raster.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance from a WIC bitmap source.
/// </summary>
HRESULT Raster::Initialize(IWICBitmapSource * bitmapSource) noexcept
{
    // Create the bitmap from the image frame.
    HRESULT hr = _WIC.CreateBitmapFromSource(bitmapSource, WICBitmapCacheOnDemand, &_Bitmap);

    if (SUCCEEDED(hr))
        hr = _Bitmap->GetSize(&_Width, &_Height);

    // Lock the complete bitmap.
    if (SUCCEEDED(hr))
    {
        WICRect LockRect = { 0, 0, (INT) _Width, (INT) _Height };

        hr = _Bitmap->Lock(&LockRect, WICBitmapLockRead, &_Lock);
    }

    if (SUCCEEDED(hr))
        hr = _Lock->GetDataPointer(&_Size, &_Data);

    if (SUCCEEDED(hr))
        hr = _Lock->GetStride(&_Stride);

    if (SUCCEEDED(hr))
        hr = _Lock->GetPixelFormat(&_PixelFormat);

    if (SUCCEEDED(hr))
        hr = _WIC.GetBitsPerPixel(_PixelFormat, _BitsPerPixel);

    return hr;
}

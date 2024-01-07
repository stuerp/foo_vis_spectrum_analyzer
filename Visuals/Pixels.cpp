
/** $VER: Pixels.cpp (2024.01.07) P. Stuer - Provides access to the pixels of a WIC image. **/

#include "Pixels.h"

#include "WIC.h"

#pragma hdrstop

HRESULT Pixels::Initialize(CComPtr<IWICBitmapFrameDecode> frame) noexcept
{
    // Create the bitmap from the image frame.
    HRESULT hr = _WIC.CreateBitmapFromSource(frame, WICBitmapCacheOnDemand, &_Bitmap);

    if (SUCCEEDED(hr))
        _Bitmap->GetSize(&_Width, &_Height);

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

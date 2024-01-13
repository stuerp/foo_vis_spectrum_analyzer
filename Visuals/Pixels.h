
/** $VER: Pixels.h (2024.01.07) P. Stuer - Provides access to the pixels of a WIC image. **/

#pragma once

#include "framework.h"

class Pixels
{
public:
    Pixels() : _Width(), _Height(), _Data(), _Size(), _Stride(), _PixelFormat(), _BitsPerPixel() { }

    HRESULT Initialize(CComPtr<IWICBitmapFrameDecode> frame) noexcept;

    UINT GetWidth() const { return _Width; }
    UINT GetHeight() const { return _Height; }
    UINT GetBitsPerPixel() const { return _BitsPerPixel; }

    BYTE * GetData() const { return _Data; }

    UINT GetStride() const { return _Stride; }
    const WICPixelFormatGUID & GetPixelFormat() const { return _PixelFormat; }

private:
    UINT _Width;
    UINT _Height;

    BYTE * _Data;
    UINT _Size;

    UINT _Stride;
    WICPixelFormatGUID _PixelFormat;
    UINT _BitsPerPixel;

    CComPtr<IWICBitmap> _Bitmap;
    CComPtr<IWICBitmapLock> _Lock;
};

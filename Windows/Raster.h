
/** $VER: Raster.h (2024.01.16) P. Stuer **/

#pragma once

#include "framework.h"

/// <summary>
/// Represents a bitmap image.
/// </summary>
class Raster
{
public:
    Raster() : _Width(), _Height(), _Data(), _Size(), _Stride(), _PixelFormat(), _BitsPerPixel() { }

    HRESULT Initialize(IWICBitmapSource * source) noexcept;

    UINT Width() const { return _Width; }
    UINT Height() const { return _Height; }

    BYTE * Data() const { return _Data; }
    UINT Size() const { return _Size; }

    UINT Stride() const { return _Stride; }
    const WICPixelFormatGUID & Format() const { return _PixelFormat; }
    UINT BitsPerPixel() const { return _BitsPerPixel; }

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

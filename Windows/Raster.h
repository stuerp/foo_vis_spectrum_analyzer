
/** $VER: Raster.h (2025.09.17) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <atlbase.h>

#include "WIC.h"

/// <summary>
/// Represents a bitmap image.
/// </summary>
class raster_t
{
public:
    raster_t() noexcept : Width(), Height(), Data(), Size(), Stride(), PixelFormat(), BitsPerPixel() { }

    HRESULT Initialize(IWICBitmapSource * source) noexcept;

public:
    UINT Width;
    UINT Height;

    BYTE * Data;
    UINT Size;

    UINT Stride;
    WICPixelFormatGUID PixelFormat;
    UINT BitsPerPixel;

private:
    CComPtr<IWICBitmap> _Bitmap;
    CComPtr<IWICBitmapLock> _Lock;
};


/** $VER: Artwork.cpp (2024.01.28) P. Stuer **/

#include "Artwork.h"

#include "WIC.h"
#include "ColorThief.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT Artwork::Initialize(const uint8_t * data, size_t size) noexcept
{
    Release();

    if ((data != nullptr) && (size != 0))
    {
        _Raster.assign(data, data + size);
        _FilePath.clear();
    }

    return S_OK;
}

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT Artwork::Initialize(const std::wstring & filePath) noexcept
{
    Release();

    _FilePath = filePath;
    _Raster.clear();

    return S_OK;
}

/// <summary>
/// Realizes this instance.
/// </summary>
HRESULT Artwork::Realize(ID2D1RenderTarget * renderTarget) noexcept
{
    if (_Raster.empty() && _FilePath.empty())
        return S_OK;

    // Load the frame from the raster data.
    HRESULT hr = !_Raster.empty() ? _WIC.Load(_Raster.data(), _Raster.size(), &_Frame) : _WIC.Load(_FilePath, &_Frame);

    // Create a format coverter to 32bppPBGRA.
    if (SUCCEEDED(hr))
    {
        hr = _WIC.Factory->CreateFormatConverter(&_FormatConverter);

        if (SUCCEEDED(hr))
            hr = _FormatConverter->Initialize(_Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
    }

    // Create a Direct2D bitmap from the WIC bitmap source.
    if (SUCCEEDED(hr))
        hr = renderTarget->CreateBitmapFromWicBitmap(_FormatConverter, nullptr, &_Bitmap);

    return hr;
}

/// <summary>
/// Creates a palette from the specified bitmap source.
/// </summary>
HRESULT Artwork::GetColors(std::vector<D2D1_COLOR_F> & colors, uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold) const noexcept
{
    if (_FormatConverter == nullptr)
        return E_FAIL;

    UINT Width = 0, Height = 0;

    HRESULT hr = _FormatConverter->GetSize(&Width, &Height);

    std::vector<ColorThief::color_t> Palette;

    if (SUCCEEDED(hr))
    {
        uint32_t Quality = Clamp((Width * Height * ColorThief::DefaultQuality) / (640 * 480), 1U, 16U); // Reference: 640 x 480 => Quality = 10

        hr = ColorThief::GetPalette(_FormatConverter, Palette, colorCount, Quality, true, (uint8_t) (lightnessThreshold * 255.f), (uint8_t) (transparencyThreshold * 255.f));
    }

    // Convert to Direct2D colors.
    if (SUCCEEDED(hr))
    {
        colors.clear();

        for (const auto & p : Palette)
            colors.push_back(D2D1::ColorF(p[0] / 255.f, p[1] / 255.f, p[2] / 255.f));
    }

    return hr;
}

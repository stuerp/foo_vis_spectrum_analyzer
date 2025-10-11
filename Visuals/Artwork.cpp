
/** $VER: Artwork.cpp (2025.09.24) P. Stuer **/

#include "pch.h"
#include "Artwork.h"

#include "WIC.h"
#include "ColorThief.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Creates the WIC resources.
/// </summary>
HRESULT artwork_t::CreateWICResources(const uint8_t * data, size_t size) noexcept
{
    _CriticalSection.Enter();

    ReleaseDeviceSpecificResources();

    if ((data != nullptr) && (size != 0))
    {
        _Raster.assign(data, data + size);
        _FilePath.clear();

        _FormatConverter.Release();
        _Frame.Release();
    }

    HRESULT hr = S_OK;

    if (_Frame == nullptr)
        hr = _WIC.Load(_Raster.data(), _Raster.size(), &_Frame);

    // Create a format converter to 32bppPBGRA.
    if (SUCCEEDED(hr) && (_FormatConverter == nullptr))
    {
        hr = _WIC.Factory->CreateFormatConverter(&_FormatConverter);

        if (SUCCEEDED(hr))
            hr = _FormatConverter->Initialize(_Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
    }

    SetStatus(Initialized);

    _CriticalSection.Leave();

    return hr;
}

/// <summary>
/// Creates the WIC resources.
/// </summary>
HRESULT artwork_t::CreateWICResources(const std::wstring & filePath) noexcept
{
    _CriticalSection.Enter();

    ReleaseDeviceSpecificResources();

    _FilePath = filePath;
    _Raster.clear();

    _FormatConverter.Release();
    _Frame.Release();

    HRESULT hr = S_OK;

    if (_Frame == nullptr)
        hr = _WIC.Load(_FilePath, &_Frame);

    // Create a format converter to 32bppPBGRA.
    if (SUCCEEDED(hr) && (_FormatConverter == nullptr))
    {
        hr = _WIC.Factory->CreateFormatConverter(&_FormatConverter);

        if (SUCCEEDED(hr))
            hr = _FormatConverter->Initialize(_Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
    }

    SetStatus(Initialized);

    _CriticalSection.Leave();

    return S_OK;
}

/// <summary>
/// Releases the WIC resources.
/// </summary>
HRESULT artwork_t::ReleaseWICResources() noexcept
{
    _CriticalSection.Enter();

    ReleaseDeviceSpecificResources();

    _FormatConverter.Release();
    _Frame.Release();

    _FilePath.clear();

    std::vector<uint8_t> Empty;

    _Raster.swap(Empty);

    SetStatus(Idle);

    _CriticalSection.Leave();

    return S_OK;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void artwork_t::Render(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & bounds, const state_t * state) noexcept
{
    _CriticalSection.Enter();

    if (_Bitmap != nullptr)
    {
        D2D1_RECT_F Rect = bounds;
        D2D1_SIZE_F Size = _Bitmap->GetSize();

        const FLOAT MaxWidth  = Rect.right  - Rect.left;
        const FLOAT MaxHeight = Rect.bottom - Rect.top;

        FLOAT WScalar = 1.f;
        FLOAT HScalar = 1.f;

        FLOAT Scalar = 1.f;

        if (state->_FitMode != FitMode::Fill)
        {
            if ((state->_FitMode == FitMode::FitWidth) || (state->_FitMode == FitMode::FitBig))
                WScalar = (Size.width  > MaxWidth)  ? (FLOAT) MaxWidth  / (FLOAT) Size.width  : 1.f;

            if ((state->_FitMode == FitMode::FitHeight) || (state->_FitMode == FitMode::FitBig))
                HScalar = (Size.height > MaxHeight) ? (FLOAT) MaxHeight / (FLOAT) Size.height : 1.f;

            Scalar = std::min(WScalar, HScalar);
        }
        else
        {
            WScalar = (Size.width  > MaxWidth)  ? (FLOAT) Size.width  / (FLOAT) MaxWidth  : (FLOAT) MaxWidth  / (FLOAT) Size.width;
            HScalar = (Size.height > MaxHeight) ? (FLOAT) Size.height / (FLOAT) MaxHeight : (FLOAT) MaxHeight / (FLOAT) Size.height;

            Scalar = std::max(WScalar, HScalar);
        }

        Size.width  *= Scalar;
        Size.height *= Scalar;

        Rect.left   += (MaxWidth  - Size.width)  / 2.f;
        Rect.top    += (MaxHeight - Size.height) / 2.f;
        Rect.right   = Rect.left + Size.width;
        Rect.bottom  = Rect.top  + Size.height;

        deviceContext->DrawBitmap(_Bitmap, Rect, state->_ArtworkOpacity, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    _CriticalSection.Leave();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT artwork_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    _CriticalSection.Enter();

    HRESULT hr = S_OK;

    // Create a Direct2D bitmap from the WIC bitmap source.
    if ((_FormatConverter != nullptr) && (_Bitmap == nullptr))
    {
        hr = deviceContext->CreateBitmapFromWicBitmap(_FormatConverter, nullptr, &_Bitmap);

        if (SUCCEEDED(hr))
            SetStatus(Realized);
    }

    _CriticalSection.Leave();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void artwork_t::ReleaseDeviceSpecificResources() noexcept
{
    _CriticalSection.Enter();

    _Bitmap.Release();

    SetStatus(Initialized);

    _CriticalSection.Leave();
}

/// <summary>
/// Creates a palette from the specified bitmap source.
/// </summary>
HRESULT artwork_t::GetColors(std::vector<D2D1_COLOR_F> & colors, uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold) noexcept
{
    HRESULT hr = E_FAIL;

    _CriticalSection.Enter();

    if (_FormatConverter != nullptr)
    {
        UINT Width = 0, Height = 0;

        hr = _FormatConverter->GetSize(&Width, &Height);

        std::vector<ColorThief::color_t> Palette;

        if (SUCCEEDED(hr))
        {
            uint32_t Quality = std::clamp((Width * Height * ColorThief::DefaultQuality) / (640 * 480), 1U, 16U); // Reference: 640 x 480 => Quality = 10

            hr = ColorThief::GetPalette(_FormatConverter, Palette, colorCount, Quality, true, (uint8_t) (lightnessThreshold * 255.f), (uint8_t) (transparencyThreshold * 255.f));
        }

        // Convert to Direct2D colors.
        if (SUCCEEDED(hr))
        {
            size_t i = 0;

            colors.resize(Palette.size());

            for (const auto & p : Palette)
                colors[i++] = D2D1::ColorF(p[0] / 255.f, p[1] / 255.f, p[2] / 255.f);
        }
    }
    else
    {
        colors.clear();

        colors.push_back(D2D1::ColorF(1.f, 0.f, 0.f));
    }

    SetStatus(GotColors);

    _CriticalSection.Leave();

    return hr;
}

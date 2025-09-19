
/** $VER: Artwork.cpp (2024.04.06) P. Stuer **/

#include "pch.h"
#include "Artwork.h"

#include "WIC.h"
#include "ColorThief.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT artwork_t::Initialize(const uint8_t * data, size_t size) noexcept
{
    _CriticalSection.Enter();

    Release();

    if ((data != nullptr) && (size != 0))
    {
        _Raster.assign(data, data + size);
        _FilePath.clear();
    }

    SetStatus(Initialized);

    _CriticalSection.Leave();

    return S_OK;
}

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT artwork_t::Initialize(const std::wstring & filePath) noexcept
{
    _CriticalSection.Enter();

    Release();

    _FilePath = filePath;
    _Raster.clear();

    SetStatus(Initialized);

    _CriticalSection.Leave();

    return S_OK;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void artwork_t::Render(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const state_t * state) noexcept
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

        renderTarget->DrawBitmap(_Bitmap, Rect, state->_ArtworkOpacity, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    _CriticalSection.Leave();
}

/// <summary>
/// Realizes this instance.
/// </summary>
HRESULT artwork_t::Realize(ID2D1RenderTarget * renderTarget) noexcept
{
    _CriticalSection.Enter();

    HRESULT hr = S_OK;

    if (!_Raster.empty() || !_FilePath.empty())
    {
        // Load the frame from the raster data.
        if (_Frame == nullptr)
            hr = !_Raster.empty() ? _WIC.Load(_Raster.data(), _Raster.size(), &_Frame) : _WIC.Load(_FilePath, &_Frame);

        // Create a format coverter to 32bppPBGRA.
        if (SUCCEEDED(hr) && (_FormatConverter == nullptr))
        {
            hr = _WIC.Factory->CreateFormatConverter(&_FormatConverter);

            if (SUCCEEDED(hr))
                hr = _FormatConverter->Initialize(_Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
        }

        // Create a Direct2D bitmap from the WIC bitmap source.
        if (SUCCEEDED(hr) && (_Bitmap == nullptr))
        {
            hr = renderTarget->CreateBitmapFromWicBitmap(_FormatConverter, nullptr, &_Bitmap);

            if (SUCCEEDED(hr))
                SetStatus(Realized);
        }
    }

    _CriticalSection.Leave();

    return hr;
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


/** $VER: Artwork.cpp (2024.03.18) P. Stuer **/

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
    _CriticalSection.Enter();

    Release();

    if ((data != nullptr) && (size != 0))
    {
        _Raster.assign(data, data + size);
        _FilePath.clear();
    }

    _Status = Initialized;

    _CriticalSection.Leave();

    return S_OK;
}

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT Artwork::Initialize(const std::wstring & filePath) noexcept
{
    _CriticalSection.Enter();

    Release();

    _FilePath = filePath;
    _Raster.clear();

    _Status = Initialized;

    _CriticalSection.Leave();

    return S_OK;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void Artwork::Render(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const State * state) noexcept
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

            Scalar = Min(WScalar, HScalar);
        }
        else
        {
            WScalar = (Size.width  > MaxWidth)  ? (FLOAT) MaxWidth  / (FLOAT) Size.width  : (FLOAT) Size.width  / (FLOAT) MaxWidth;
            HScalar = (Size.height > MaxHeight) ? (FLOAT) MaxHeight / (FLOAT) Size.height : (FLOAT) Size.height / (FLOAT) MaxHeight;

            Scalar = Max(WScalar, HScalar);
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
HRESULT Artwork::Realize(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    _CriticalSection.Enter();

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
                _Status = Realized;
        }
    }

    _CriticalSection.Leave();

    return hr;
}

/// <summary>
/// Creates a palette from the specified bitmap source.
/// </summary>
HRESULT Artwork::GetColors(std::vector<D2D1_COLOR_F> & colors, uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold) noexcept
{
    if (_FormatConverter == nullptr)
        return E_FAIL;

    _CriticalSection.Enter();

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

    _CriticalSection.Leave();

    return hr;
}

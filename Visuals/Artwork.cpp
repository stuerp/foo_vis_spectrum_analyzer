
/** $VER: Artwork.cpp (2026.07.22) P. Stuer **/

#include "pch.h"

#include "Artwork.h"

#include "WIC.h"
#include "ColorThief.h"
#include "Support.h"

#include <State.h>
#include <Constants.h>

#pragma hdrstop

/// <summary>
/// Creates the WIC resources.
/// </summary>
HRESULT artwork_t::CreateWICResources(const uint8_t * data, size_t size) noexcept
{
    msc::lock_t Lock(_CriticalSection);

    DeleteDeviceSpecificResources();

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

    return hr;
}

/// <summary>
/// Creates the WIC resources.
/// </summary>
HRESULT artwork_t::CreateWICResources(const std::wstring & filePath) noexcept
{
    msc::lock_t Lock(_CriticalSection);

    DeleteDeviceSpecificResources();

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

    return S_OK;
}

/// <summary>
/// Releases the WIC resources.
/// </summary>
HRESULT artwork_t::DeleteWICResources() noexcept
{
    msc::lock_t Lock(_CriticalSection);

    DeleteDeviceSpecificResources();

    _FormatConverter.Release();
    _Frame.Release();

    _FilePath.clear();

    std::vector<uint8_t> Empty;

    _Raster.swap(Empty);

    SetStatus(Idle);

    return S_OK;
}

/// <summary>
/// Creates a palette from the WIC bitmap source.
/// </summary>
HRESULT artwork_t::GetColors(std::vector<D2D1_COLOR_F> & colors, uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold) noexcept
{
    msc::lock_t Lock(_CriticalSection);

    HRESULT hr = E_FAIL;

    if (_FormatConverter != nullptr)
    {
        UINT Width = 0, Height = 0;

        hr = _FormatConverter->GetSize(&Width, &Height);

        std::vector<ColorThief::color_t> Palette;

        if (SUCCEEDED(hr))
        {
            const uint32_t Quality = std::clamp((Width * Height * ColorThief::DefaultQuality) / (640 * 480), 1U, 16U); // Reference: 640 x 480 => Quality = 10

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

    return hr;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void artwork_t::Render(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & rect, const state_t * state) noexcept
{
    msc::lock_t Lock(_CriticalSection);

    if (_Bitmap == nullptr)
        return;

    FLOAT Scalar = 1.f;
    D2D1_RECT_F Rect = rect;

    AdjustRect(state->_FitMode, Scalar, Rect);

    // When the blur effect chain is unavailable (e.g. under Wine/Proton), fall back to a plain bitmap draw. This still honors the opacity but loses the blur.
    if ((state->_ArtworkBlurSigma == 0.f) || !_HasEffects)
    {
        deviceContext->DrawBitmap(_Bitmap, Rect, state->_ArtworkOpacity, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }
    else
    {
        FLOAT DPIX, DPIY;

        deviceContext->GetDpi(&DPIX, &DPIY);

        const FLOAT DPIScale = DPIX / 96.f;

        _ScaleEffect->SetInput(0, _Bitmap);
        _ScaleEffect->SetValue(D2D1_SCALE_PROP_SCALE, D2D1::Vector2F(Scalar * DPIScale, Scalar * DPIScale));

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, state->_ArtworkBlurSigma);

        _OpacityEffect->SetValue(D2D1_OPACITY_PROP_OPACITY, state->_ArtworkOpacity);

        const D2D1_POINT_2F Offset = { Rect.left, Rect.top };

        deviceContext->DrawImage(_OpacityEffect, Offset);
    }
}

/// <summary>
/// Adjusts the bitmap destination rectangle depending on the selected fit mode.
/// </summary>
void artwork_t::AdjustRect(_In_ const FitMode fitMode, _Out_ FLOAT & scalar, _Inout_ D2D1_RECT_F & rect) const noexcept
{
    const FLOAT MaxWidth  = rect.right  - rect.left;
    const FLOAT MaxHeight = rect.bottom - rect.top;

    FLOAT WScalar = 1.f, HScalar = 1.f;

    D2D1_SIZE_F Size = _Bitmap->GetSize();

    if (fitMode != FitMode::Fill)
    {
        if ((fitMode == FitMode::FitWidth) || (fitMode == FitMode::FitBig))
            WScalar = (Size.width  > MaxWidth)  ? MaxWidth  / Size.width  : 1.f;

        if ((fitMode == FitMode::FitHeight) || (fitMode == FitMode::FitBig))
            HScalar = (Size.height > MaxHeight) ? MaxHeight / Size.height : 1.f;

        scalar = std::min(WScalar, HScalar);
    }
    else
    {
        WScalar = (Size.width  > MaxWidth)  ? Size.width  / MaxWidth  : MaxWidth  / Size.width;
        HScalar = (Size.height > MaxHeight) ? Size.height / MaxHeight : MaxHeight / Size.height;

        scalar = std::max(WScalar, HScalar);
    }

    Size.width  *= scalar;
    Size.height *= scalar;

    rect.left   += (MaxWidth  - Size.width)  / 2.f;
    rect.top    += (MaxHeight - Size.height) / 2.f;
    rect.right   = rect.left + Size.width;
    rect.bottom  = rect.top  + Size.height;
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT artwork_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    {
        msc::lock_t Lock(_CriticalSection);

        // No format converter means no artwork.
        if (_FormatConverter == nullptr)
            return E_FAIL;

        // Create a Direct2D bitmap from the WIC bitmap source.
        if (_Bitmap == nullptr)
        {
            hr = deviceContext->CreateBitmapFromWicBitmap(_FormatConverter, nullptr, &_Bitmap);

            if (SUCCEEDED(hr))
                SetStatus(GotBitmap);
        }
    }

    // Without a bitmap there is nothing to render or to attach effects to.
    if (_Bitmap == nullptr)
        return SUCCEEDED(hr) ? E_FAIL : hr;

    // The Scale/GaussianBlur/Opacity effect chain is only used to render a blurred background. Direct2D effects are not (well) supported under
    // Wine/Proton, so create them only when available and fall back to an unblurred DrawBitmap() (see Render()) otherwise.
    if (!IsWineOrProton() && (_ScaleEffect == nullptr))
    {
        hr = deviceContext->CreateEffect(CLSID_D2D1Scale, &_ScaleEffect);

        if (SUCCEEDED(hr))
            hr = deviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &_BlurEffect);

        if (SUCCEEDED(hr))
        {
            _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_DIRECTIONALBLUR_OPTIMIZATION_QUALITY);
            _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

            _BlurEffect->SetInputEffect(0, _ScaleEffect);

            hr = deviceContext->CreateEffect(CLSID_D2D1Opacity, &_OpacityEffect);
        }

        if (SUCCEEDED(hr))
        {
            _OpacityEffect->SetInputEffect(0, _BlurEffect);
        }
        else
        {
            // Effects unavailable. Release any partial chain and render without blur.
            _OpacityEffect.Release();
            _BlurEffect.Release();
            _ScaleEffect.Release();
        }
    }

    _HasEffects = (_ScaleEffect != nullptr) && (_BlurEffect != nullptr) && (_OpacityEffect != nullptr);

    return S_OK;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void artwork_t::DeleteDeviceSpecificResources() noexcept
{
    _OpacityEffect.Release();
    _BlurEffect.Release();
    _ScaleEffect.Release();

    {
        msc::lock_t Lock(_CriticalSection);

        _Bitmap.Release();

        SetStatus(Initialized);
    }
}

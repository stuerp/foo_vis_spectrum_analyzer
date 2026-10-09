
/** $VER: Artwork.cpp (2026.10.09) P. Stuer **/

#include "pch.h"

#include "Artwork.h"

#include "WIC.h"
#include "ColorThief.h"
#include "Resources.h"

#include <State.h>
#include <Constants.h>

#pragma hdrstop

/// <summary>
/// Creates the WIC resources.
/// </summary>
HRESULT artwork_t::CreateWICResources(const uint8_t * data, size_t size) noexcept
{
    assert(core_api::is_main_thread());

    msc::lock_t Lock(_CriticalSection);

    DeleteDeviceSpecificResources();

    if ((data != nullptr) && (size != 0))
    {
        _Raster.assign(data, data + size);
        _FilePath.clear();

        _FormatConverter.Reset();
        _Frame.Reset();
    }

    HRESULT hr = S_OK;

    if (_Frame == nullptr)
    {
        hr = WIC::Load(_Raster.data(), _Raster.size(), _Frame.GetAddressOf());

        if (FAILED(hr))
            return hr;

        // Create a format converter to 32bppPBGRA.
        if (_FormatConverter == nullptr)
        {
            hr = WICFactory::Get()->CreateFormatConverter(_FormatConverter.GetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        hr = _FormatConverter->Initialize(_Frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
    }

    return hr;
}

/// <summary>
/// Creates the WIC resources.
/// </summary>
HRESULT artwork_t::CreateWICResources(const std::wstring & filePath) noexcept
{
    assert(core_api::is_main_thread());

    msc::lock_t Lock(_CriticalSection);

    DeleteDeviceSpecificResources();

    _FilePath = filePath;
    _Raster.clear();

    _FormatConverter.Reset();
    _Frame.Reset();

    HRESULT hr = S_OK;

    if (_Frame == nullptr)
    {
        hr = WIC::Load(_FilePath, _Frame.GetAddressOf());

        if (FAILED(hr))
            return hr;

        // Create a format converter to 32bppPBGRA.
        if (_FormatConverter == nullptr)
        {
            hr = WICFactory::Get()->CreateFormatConverter(_FormatConverter.GetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        hr = _FormatConverter->Initialize(_Frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
    }

    return S_OK;
}

/// <summary>
/// Releases the WIC resources.
/// </summary>
HRESULT artwork_t::DeleteWICResources() noexcept
{
    assert(core_api::is_main_thread());

    msc::lock_t Lock(_CriticalSection);

    DeleteDeviceSpecificResources();

    _FormatConverter.Reset();
    _Frame.Reset();

    _FilePath.clear();

    std::vector<uint8_t> Empty;

    _Raster.swap(Empty);

    return S_OK;
}

/// <summary>
/// Creates a palette from the WIC bitmap source.
/// </summary>
HRESULT artwork_t::GetColors(uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold, std::vector<D2D1_COLOR_F> & colors) noexcept
{
    msc::lock_t Lock(_CriticalSection);

    try
    {
        const auto SetErrorColor = [&colors]()
        {
            colors.assign(1, D2D1::ColorF(D2D1::ColorF::Red)); // Makes an error easier to detect.
        };

        if (_FormatConverter == nullptr)
        {
            SetErrorColor();

            return S_FALSE;
        }

        UINT Width = 0, Height = 0;

        HRESULT hr = _FormatConverter->GetSize(&Width, &Height);

        if (FAILED(hr))
            return hr;

        if (Width == 0 || Height == 0 || colorCount == 0)
        {
            SetErrorColor();

            return S_FALSE;
        }

        constexpr uint64_t ReferencePixelCount = 640ULL * 480ULL; // Reference: 640 x 480 => Quality = 10
        constexpr uint32_t MinimumQuality      =  1;
        constexpr uint32_t MaximumQuality      = 16;

        const uint64_t PixelCount    = (uint64_t) Width * (uint64_t) Height;
        const uint64_t ScaledQuality = PixelCount * (uint64_t) (ColorThief::DefaultQuality) / ReferencePixelCount;

        const uint32_t Quality = (uint32_t) std::clamp<uint64_t>(ScaledQuality, MinimumQuality, MaximumQuality);

        const auto ToByte = [](const FLOAT value) noexcept -> uint8_t
        {
            const FLOAT Normalized = std::clamp(value, 0.f, 1.f);

            return static_cast<uint8_t>(Normalized * 255.f + 0.5f);
        };

        std::vector<ColorThief::color_t> Palette;

        hr = ColorThief::GetPalette(_FormatConverter.Get(), colorCount, Quality, true, ToByte(lightnessThreshold), ToByte(transparencyThreshold), Palette);

        if (FAILED(hr))
            return hr;

        // Convert to Direct2D colors.
        {
            std::vector<D2D1_COLOR_F> Result(Palette.size());

            size_t i = 0;

            for (const auto & Color : Palette)
                Result[i++] = D2D1::ColorF(Color[0] / 255.f, Color[1] / 255.f, Color[2] / 255.f);

            colors = std::move(Result); // Atomic update
        }

        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void artwork_t::Render(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & rect, const state_t * state) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    msc::lock_t Lock(_CriticalSection);

    if (_Bitmap == nullptr)
        return;

    FLOAT Scalar = 1.f;
    D2D1_RECT_F Rect = rect;

    AdjustRect(state->_FitMode, state->_AllowUpscaling, Scalar, Rect);

    if (state->_ArtworkBlurSigma == 0.f)
    {
        deviceContext->DrawBitmap(_Bitmap.Get(), Rect, state->_ArtworkOpacity, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }
    else
    {
        FLOAT DPIX, DPIY;

        deviceContext->GetDpi(&DPIX, &DPIY);

        const FLOAT DPIScale = DPIX / 96.f;

        _ScaleEffect->SetInput(0, _Bitmap.Get());
        _ScaleEffect->SetValue(D2D1_SCALE_PROP_SCALE, D2D1::Vector2F(Scalar * DPIScale, Scalar * DPIScale));

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, state->_ArtworkBlurSigma);

        _OpacityEffect->SetValue(D2D1_OPACITY_PROP_OPACITY, state->_ArtworkOpacity);

        const D2D1_POINT_2F Offset = { Rect.left, Rect.top };

        deviceContext->DrawImage(_OpacityEffect.Get(), Offset);
    }
}

/// <summary>
/// Adjusts the bitmap destination rectangle depending on the selected fit mode.
/// </summary>
/*
void artwork_t::AdjustRect(const FitMode fitMode, FLOAT & scalar, D2D1_RECT_F & rect) const noexcept
{
    scalar = 1.f;

    D2D1_SIZE_F Size = _Bitmap->GetSize();

    if ((Size.width == 0) || (Size.height == 0))
        return;

    const FLOAT AreaWidth  = rect.right  - rect.left;
    const FLOAT AreaHeight = rect.bottom - rect.top;

    FLOAT WScalar = 1.f, HScalar = 1.f;

    if (msc::InRange(fitMode, FitMode::Free, FitMode::FitHeight))
    {
        if ((fitMode == FitMode::FitWidth) || (fitMode == FitMode::FitLargest))
            WScalar = (Size.width  > AreaWidth)  ? AreaWidth  / Size.width  : 1.f;

        if ((fitMode == FitMode::FitHeight) || (fitMode == FitMode::FitLargest))
            HScalar = (Size.height > AreaHeight) ? AreaHeight / Size.height : 1.f;

        scalar = std::min(WScalar, HScalar);
    }
    else
    {
        WScalar = (Size.width  > AreaWidth)  ? Size.width  / AreaWidth  : AreaWidth  / Size.width;
        HScalar = (Size.height > AreaHeight) ? Size.height / AreaHeight : AreaHeight / Size.height;

        scalar = std::max(WScalar, HScalar);
    }

    Size.width  *= scalar;
    Size.height *= scalar;

    rect.left   += (AreaWidth  - Size.width)  / 2.f;
    rect.top    += (AreaHeight - Size.height) / 2.f;
    rect.right   = rect.left + Size.width;
    rect.bottom  = rect.top  + Size.height;
}
*/
void artwork_t::AdjustRect(const FitMode fitMode, const bool allowUpscaling, FLOAT & scalar, D2D1_RECT_F & rect) const noexcept
{
    scalar = 1.0f;

    if (_Bitmap == nullptr)
        return;

    const D2D1_SIZE_F Size = _Bitmap->GetSize();

    if (Size.width <= 0.0f || Size.height <= 0.0f)
        return;

    const FLOAT AreaWidth  = rect.right  - rect.left;
    const FLOAT AreaHeight = rect.bottom - rect.top;

    if (AreaWidth <= 0.f || AreaHeight <= 0.f)
        return;

    switch (fitMode)
    {
        case FitMode::FitWidth:
        {
            scalar = AreaWidth / Size.width;
            break;
        }

        case FitMode::FitHeight:
        {
            scalar = AreaHeight / Size.height;
            break;
        }

        case FitMode::FitSmallest:
        {
            scalar = Size.width <= Size.height ? AreaWidth / Size.width : AreaHeight / Size.height;
            break;
        }

        case FitMode::FitLargest:
        {
            scalar = Size.width >= Size.height ? AreaWidth / Size.width : AreaHeight / Size.height;
            break;
        }

        case FitMode::Free:
        default:
        {
            scalar = 1.0f;
            break;
        }
    }

    if (!allowUpscaling)
        scalar = std::min(scalar, 1.0f);

    const FLOAT ScaledWidth  = Size.width * scalar;
    const FLOAT ScaledHeight = Size.height * scalar;

    const FLOAT Left = rect.left + (AreaWidth  - ScaledWidth)  / 2.f;
    const FLOAT Top  = rect.top  + (AreaHeight - ScaledHeight) / 2.f;

    rect = D2D1::RectF(Left, Top, Left + ScaledWidth, Top + ScaledHeight);
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
            hr = deviceContext->CreateBitmapFromWicBitmap(_FormatConverter.Get(), nullptr, _Bitmap.GetAddressOf());

            if (FAILED(hr))
            {
                Log.AtWarn().Write(STR_COMPONENT_BASENAME " failed to create Direct2D bitmap from WIC bitmap: 0x%08X", hr);

                return hr;
            }
        }
    }

    if (_ScaleEffect == nullptr)
    {
        hr = deviceContext->CreateEffect(CLSID_D2D1Scale, _ScaleEffect.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    if (_BlurEffect == nullptr)
    {
        hr = deviceContext->CreateEffect(CLSID_D2D1GaussianBlur, _BlurEffect.GetAddressOf());

        if (FAILED(hr))
            return hr;

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_DIRECTIONALBLUR_OPTIMIZATION_BALANCED);
        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

        _BlurEffect->SetInputEffect(0, _ScaleEffect.Get());
    }

    if (_OpacityEffect == nullptr)
    {
        hr = deviceContext->CreateEffect(CLSID_D2D1Opacity, _OpacityEffect.GetAddressOf());

        if (FAILED(hr))
            return hr;

        _OpacityEffect->SetInputEffect(0, _BlurEffect.Get());
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void artwork_t::DeleteDeviceSpecificResources() noexcept
{
    _OpacityEffect.Reset();
    _BlurEffect.Reset();
    _ScaleEffect.Reset();

    {
        msc::lock_t Lock(_CriticalSection);

        _Bitmap.Reset();
    }
}

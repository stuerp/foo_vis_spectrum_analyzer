
/** $VER: OscilloscopeBase.cpp (2026.10.07) P. Stuer - Implements a base class for an oscilloscope. **/

#include <pch.h>

#include "OscilloscopeBase.h"

#include "Direct2D.h"

#pragma hdrstop

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_base_t::~oscilloscope_base_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT oscilloscope_base_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    // Create a brush stroke style for the signal.
    if (_SignalStrokeStyle == nullptr)
    {
        const D2D1_STROKE_STYLE_PROPERTIES StrokeStyleProperties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_BEVEL);

        hr = Direct2DFactory::Get()->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, _SignalStrokeStyle.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    // Create a brush stroke style for the static content that remains fixed during the scaling transformation.
    if (_StaticStrokeStyle == nullptr)
    {
        D2D1_STROKE_STYLE_PROPERTIES1 StrokeStyleProperties = D2D1::StrokeStyleProperties1();

        StrokeStyleProperties.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED; // Prevent stroke scaling

        hr = Direct2DFactory::Get()->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, _StaticStrokeStyle.GetAddressOf());
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_base_t::DeleteDeviceIndependentResources() noexcept
{
    _StaticStrokeStyle.Reset();
    _SignalStrokeStyle.Reset();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_base_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_ResizeResources)
        DeleteDeviceSpecificResources();

    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _DebugBrush.GetAddressOf());
#endif

    if (_DeviceContext == nullptr)
    {
        ComPtr<ID2D1Device> D2DDevice;

        deviceContext->GetDevice(D2DDevice.GetAddressOf());

        hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, _DeviceContext.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    if (_OpacityEffect == nullptr)
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1Opacity, _OpacityEffect.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    if (_BlurEffect == nullptr)
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1GaussianBlur, _BlurEffect.GetAddressOf());

        if (FAILED(hr))
            return hr;

        _BlurEffect->SetInputEffect(0, _OpacityEffect.Get());

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_DIRECTIONALBLUR_OPTIMIZATION_BALANCED);
        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
    }

    hr = CreateSizeDependentResources(deviceContext);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void oscilloscope_base_t::DeleteDeviceSpecificResources() noexcept
{
    DeleteSizeDependentResources();

    _BlurEffect.Reset();
    _OpacityEffect.Reset();

    _DeviceContext.Reset();

#ifdef _DEBUG
    _DebugBrush.Reset();
#endif
}

/// <summary>
/// Creates the resources that depend on the size of the front buffer of the device context.
/// </summary>
HRESULT oscilloscope_base_t::CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if ((_Size.width <= 0.f) || _Size.height <= 0.f)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    const auto CreateStyle = [this, deviceContext](auto & Style, VisualElement Element) noexcept -> HRESULT
    {
        if (Style._Brush != nullptr)
            return S_OK;

        Style = *_State->_StyleManager.GetStyle(Element);

        Style.SetColor(_State);

        return Style.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);
    };

    hr = CreateStyle(_SignalLineStyle, VisualElement::Signal);

    if (FAILED(hr))
        return hr;

    hr = CreateStyle(_XAxisLineStyle, VisualElement::XAxisLine);

    if (FAILED(hr))
        return hr;

    hr = CreateStyle(_YAxisLineStyle, VisualElement::YAxisLine);

    if (FAILED(hr))
        return hr;

    hr = CreateStyle(_HorizontalGridLineStyle, VisualElement::HorizontalGridLine);

    if (FAILED(hr))
        return hr;

    hr = CreateStyle(_VerticalGridLineStyle, VisualElement::VerticalGridLine);

    if (FAILED(hr))
        return hr;

    if ((_Bitmaps[0] == nullptr) || (_Bitmaps[1] == nullptr))
    {
        const D2D1_BITMAP_PROPERTIES1 BitmapProperties = D2D1::BitmapProperties1
        (
            D2D1_BITMAP_OPTIONS_TARGET,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED) // Required for alpha transparency. Otherwise use D2D1_ALPHA_MODE_IGNORE.
        );

        UINT32 w = (UINT32) _Size.width;
        UINT32 h = (UINT32) _Size.height;

        if (_SquareBitmaps)
        {
//          _Side = std::min(_Size.width, _Size.height);

            const FLOAT x = (_Size.width  - _Side) / 2.f;
            const FLOAT y = (_Size.height - _Side) / 2.f;

            _DestinationRectangle = { x, y, x + _Side, y + _Side };

            w = h = (UINT) _Side;
        }

        if (_Bitmaps[0] == nullptr)
        {
            hr = deviceContext->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, &BitmapProperties, _Bitmaps[0].GetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        if (_Bitmaps[1] == nullptr)
        {
            hr = deviceContext->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, &BitmapProperties, _Bitmaps[1].GetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        hr = ClearBitmaps();

        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

/// <summary>
/// Deletes the resources that depend on the size of the front buffer of the device context.
/// </summary>
void oscilloscope_base_t::DeleteSizeDependentResources() noexcept
{
    _Bitmaps[1].Reset();
    _Bitmaps[0].Reset();

    _SignalLineStyle.DeleteDeviceSpecificResources();
    _XAxisLineStyle.DeleteDeviceSpecificResources();
    _YAxisLineStyle.DeleteDeviceSpecificResources();
    _HorizontalGridLineStyle.DeleteDeviceSpecificResources();
    _VerticalGridLineStyle.DeleteDeviceSpecificResources();
}

/// <summary>
/// Clears the back buffers.
/// </summary>
HRESULT oscilloscope_base_t::ClearBitmaps() noexcept
{
    _DeviceContext->BeginDraw();

    for (const auto & Bitmap : _Bitmaps)
    {
        if (Bitmap == nullptr)
            continue;

        _DeviceContext->SetTarget(Bitmap.Get());

        _DeviceContext->Clear(); // Transparent
    }

    _DeviceContext->SetTarget(nullptr);

    HRESULT hr = _DeviceContext->EndDraw();

    return hr;
}

/// <summary>
/// Finds the zero-crossing in the chunk.
/// </summary>
size_t oscilloscope_base_t::FindZeroCrossing(const audio_sample * frames, size_t frameCount, uint32_t channelCount) noexcept
{
    if ((frames == nullptr) || (frameCount < 3) || (channelCount == 0))
        return frameCount;

    size_t CrossIndex = frameCount;

    // Return the earliest zero-crossing across all channels.
    for (size_t i = 0; i < channelCount; ++i)
    {
        const audio_sample * Frame = frames + i;

        audio_sample Sample0 = frames[0];
        audio_sample Sample1 = frames[channelCount];

        for (size_t j = 2; (j < frameCount) && (j <= CrossIndex); ++j)
        {
            Frame += channelCount;

            const audio_sample Sample2 = frames[channelCount];

            // Is this a rising zero crossing? Confirm with the next sample.
            if ((Sample0 < 0.) && (Sample1 >= 0.) && (Sample2 >= 0.))
            {
                CrossIndex = j - 1;

                if (CrossIndex == 1)
                    return CrossIndex;

                break;
            }

            Sample0 = Sample1;
            Sample1 = Sample2;
        }
    }

    return CrossIndex;
}

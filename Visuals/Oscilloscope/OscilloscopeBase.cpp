
/** $VER: OscilloscopeBase.cpp (2026.06.21) P. Stuer - Implements a base class for an oscilloscope. **/

#include <pch.h>

#include "OscilloscopeBase.h"

#include "Direct2D.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
oscilloscope_base_t::oscilloscope_base_t()
{
}

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_base_t::~oscilloscope_base_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void oscilloscope_base_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    // Release resources that are size dependent.
    _CompositeBuffer.Release();
    _BackBuffer.Release();
    _FrontBuffer.Release();

    _IsResized = false;
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT oscilloscope_base_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    // Create a brush stroke style for the signal.
    if (SUCCEEDED(hr) && (_SignalStrokeStyle == nullptr))
    {
        const D2D1_STROKE_STYLE_PROPERTIES StrokeStyleProperties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_BEVEL);

        hr = _Direct2D.Factory->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, &_SignalStrokeStyle);
    }

    // Create a brush stroke style for the axes and grid that remains fixed during the scaling transformation.
    if (SUCCEEDED(hr))
    {
        D2D1_STROKE_STYLE_PROPERTIES1 StrokeStyleProperties = D2D1::StrokeStyleProperties1();

        StrokeStyleProperties.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED; // Prevent stroke scaling

        hr = _Direct2D.Factory->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, &_AxisStrokeStyle);
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_base_t::DeleteDeviceIndependentResources() noexcept
{
    _AxisStrokeStyle.Release();

    _SignalStrokeStyle.Release();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_base_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    Resize();

    HRESULT hr = S_OK;

    if (_SignalLineStyle._Brush == nullptr)
    {
        _SignalLineStyle = *_State->_StyleManager.GetStyle(VisualElement::SignalLine);

        _SignalLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _SignalLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_XAxisLineStyle._Brush == nullptr)
    {
        _XAxisLineStyle = *_State->_StyleManager.GetStyle(VisualElement::XAxisLine);

        _XAxisLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _XAxisLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_YAxisLineStyle._Brush == nullptr)
    {
        _YAxisLineStyle = *_State->_StyleManager.GetStyle(VisualElement::YAxisLine);

        _YAxisLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _YAxisLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_HorizontalGridLineStyle._Brush == nullptr)
    {
        _HorizontalGridLineStyle = *_State->_StyleManager.GetStyle(VisualElement::HorizontalGridLine);

        _HorizontalGridLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _HorizontalGridLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_VerticalGridLineStyle._Brush == nullptr)
    {
        _VerticalGridLineStyle = *_State->_StyleManager.GetStyle(VisualElement::VerticalGridLine);

        _VerticalGridLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _VerticalGridLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_DeviceContext == nullptr)
    {
        CComPtr<ID2D1Device> D2DDevice;

        deviceContext->GetDevice(&D2DDevice);

        hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &_DeviceContext);

        if (!SUCCEEDED(hr))
            return hr;
    }

    const D2D1_BITMAP_PROPERTIES1 BitmapProperties = D2D1::BitmapProperties1
    (
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED) // Required for alpha transparency. Otherwise use D2D1_ALPHA_MODE_IGNORE.
    );

    if (_FrontBuffer == nullptr)
    {
        hr = deviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Size.width, (UINT32) _Size.height), nullptr, 0, &BitmapProperties, &_FrontBuffer);

        if (!SUCCEEDED(hr))
            return hr;

        _DeviceContext->SetTarget(_FrontBuffer);

        _DeviceContext->BeginDraw();

        _DeviceContext->Clear(_State->_HasPhosphorDecay ? D2D1::ColorF(D2D1::ColorF::Black) : D2D1::ColorF(0, 0, 0, 0)); // FIXME: Phosphor decay does not work with alpha transparency.


        hr = _DeviceContext->EndDraw();

        if (!SUCCEEDED(hr))
            return hr;

        _DeviceContext->SetTarget(nullptr);
    }

    if (_BackBuffer == nullptr)
    {
        hr = _DeviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Size.width, (UINT32) _Size.height), nullptr, 0, &BitmapProperties, &_BackBuffer);

        if (!SUCCEEDED(hr))
            return hr;

        _DeviceContext->SetTarget(_BackBuffer);

        _DeviceContext->BeginDraw();

        _DeviceContext->Clear(_State->_HasPhosphorDecay ? D2D1::ColorF(D2D1::ColorF::Black) : D2D1::ColorF(0, 0, 0, 0)); // FIXME: Phosphor decay does not work with alpha transparency.

        hr = _DeviceContext->EndDraw();

        if (!SUCCEEDED(hr))
            return hr;

        _DeviceContext->SetTarget(nullptr);
    }

    if (_CompositeBuffer == nullptr)
    {
        hr = _DeviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Size.width, (UINT32) _Size.height), nullptr, 0, &BitmapProperties, &_CompositeBuffer);

        if (!SUCCEEDED(hr))
            return hr;

        _DeviceContext->SetTarget(_CompositeBuffer);

        _DeviceContext->BeginDraw();

        _DeviceContext->Clear(_State->_HasPhosphorDecay ? D2D1::ColorF(D2D1::ColorF::Black) : D2D1::ColorF(0, 0, 0, 0)); // FIXME: Phosphor decay does not work with alpha transparency.

        hr = _DeviceContext->EndDraw();

        if (!SUCCEEDED(hr))
            return hr;

        _DeviceContext->SetTarget(nullptr);
    }

    if (_BlurEffect == nullptr)
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &_BlurEffect);

        if (!SUCCEEDED(hr))
            return hr;

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, _State->_BlurSigma);
        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_DIRECTIONALBLUR_OPTIMIZATION_QUALITY);
        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
    }

    if (_ColorMatrixEffect == nullptr)
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1ColorMatrix, &_ColorMatrixEffect);

        if (!SUCCEEDED(hr))
            return hr;

        // Color matrix for uniform decay
        #pragma warning(disable: 5246) // 'anonymous struct or union': the initialization of a subobject should be wrapped in braces
        const D2D1_MATRIX_5X4_F DecayMatrix =
        {
            _State->_DecayFactor, 0, 0, 0,  // Decay red
            0, _State->_DecayFactor, 0, 0,  // Decay green
            0, 0, _State->_DecayFactor, 0,  // Decay blue
            0, 0, 0, 1,                     // Keep alpha
            0, 0, 0, 0                      // Unused. Translation
        };

        _ColorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, DecayMatrix);
    }

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void oscilloscope_base_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    _ColorMatrixEffect.Release();

    _BlurEffect.Release();

    _CompositeBuffer.Release();
    _BackBuffer.Release();
    _FrontBuffer.Release();

    _DeviceContext.Release();

    _SignalLineStyle.DeleteDeviceSpecificResources();
    _XAxisLineStyle.DeleteDeviceSpecificResources();
    _YAxisLineStyle.DeleteDeviceSpecificResources();
    _HorizontalGridLineStyle.DeleteDeviceSpecificResources();
}


/** $VER: Goniometer.cpp (2026.09.22) P. Stuer - Implements a goniometer. **/

#include <pch.h>

#include "Goniometer.h"
#include "Direct2D.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
goniometer_t::goniometer_t()
{
    _Rect = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
goniometer_t::~goniometer_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void goniometer_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, CComPtr<ID3D11Device>, CComPtr<ID3D11DeviceContext>) noexcept
{
    _State        = state;
    _GraphOptions = graphOptions;
    _Analysis     = analysis;

    _LowBand  = _State->_LowBand;
    _HighBand = _State->_HighBand;

    _AudioProcessor.Initialize(state);

    _AudioProcessor.SetColorMode(_State->_GoniometerColorMode);
    _AudioProcessor.SetCrossoverMode(_State->_CrossoverMode);
    _AudioProcessor.SetVisualGain(_State->_LowVisualGain, _State->_MidVisualGain, _State->_HighVisualGain);

    CreateDeviceIndependentResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void goniometer_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);

    _Side              = std::min(_Size.width, _Size.height);
    _ScaleFactor       = _Side / 2.f;
    _TranslationMatrix = D2D1::Matrix3x2F::Translation(_Size.width / 2.f, _Size.height / 2.f);
}

/// <summary>
/// Resets this instance.
/// </summary>
void goniometer_t::Reset() noexcept
{
    if (_ForceElementToResize || (_Size.width <= 0.f) || (_Size.height <= 0.f))
        return;

    _SpriteDestinations.resize(0);
    _SpriteSources     .resize(0);
    _SpriteColors      .resize(0);
    _SpriteTransforms  .resize(0);

    _ForceElementToResize = true;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void goniometer_t::Release() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Handles a configuration change.
/// </summary>
void goniometer_t::OnConfigurationChange(ConfigurationChanges configurationChanges) noexcept
{
    if (!IsSet(configurationChanges, ConfigurationChanges::Goniometer))
        return;

    _AudioProcessor.SetColorMode(_State->_GoniometerColorMode);
    _AudioProcessor.SetCrossoverMode(_State->_CrossoverMode);
    _AudioProcessor.SetVisualGain(_State->_LowVisualGain, _State->_MidVisualGain, _State->_HighVisualGain);
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void goniometer_t::Resize() noexcept
{
    if (!_ForceElementToResize || (_Size.width <= 0.f) || (_Size.height <= 0.f))
        return;

    DeleteSizeDependentResources();

    _ForceElementToResize = false;
}


/// <summary>
/// Renders this instance.
/// </summary>
void goniometer_t::Render(ID2D1DeviceContext * deviceContext, CComPtr<IDXGISwapChain1> swapChain) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    // Process the chunk.
    if (!_Analysis->_Chunk.is_empty())
    {
        const uint32_t ActiveChannelMask = _GraphOptions->_ActiveChannelMask;                               // Mask containing the channels selected by the user.
        const uint32_t PairedChannelMask = analysis_t::ChannelPairs[(size_t) _GraphOptions->_ChannelPair];  // Mask containing the channels selected by the user as a channel pair.

        _AudioProcessor.Process(_Analysis->_Chunk, ActiveChannelMask, PairedChannelMask, _LowBand, _HighBand);
    }
    else
        _AudioProcessor.Reset();

    // Create the sprite batch from the audio points.
    if (_AudioProcessor._PointCount != 0)
        CreateSprites();
    else
        _SpriteBatch->Clear();

    const size_t BitmapIndex = 1 - _PrevBitmapIndex;

    // Draw the next back buffer frame.
    {
        _DeviceContext->BeginDraw();

        _DeviceContext->SetTarget(_Bitmaps[BitmapIndex].Get());

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        _DeviceContext->Clear();

        // Draw a faded and blurred version of the previous bitmap.
        {
            _OpacityEffect->SetInput(0, _Bitmaps[_PrevBitmapIndex].Get());

            _DeviceContext->DrawImage(_BlurEffect.Get());
        }

        // Draw the sprites.
        {
            if (_SpriteBatch->GetSpriteCount() != 0)
                _DeviceContext->DrawSpriteBatch(_SpriteBatch.Get(), _Sprite.Get(), D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1_SPRITE_OPTIONS_NONE);
        }

        hr = _DeviceContext->EndDraw();

        if (FAILED(hr))
            return;
    }

    {
        // Draw the static content to the front buffer.
        deviceContext->SetTransform(_TranslationMatrix);

        deviceContext->DrawImage(_StaticContent.Get());

        deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

        // Draw the composited frame to the front buffer.
        deviceContext->DrawBitmap(_Bitmaps[BitmapIndex].Get(), _DestinationRectangle, 1.f, D2D1_INTERPOLATION_MODE_LINEAR);
    }

    _PrevBitmapIndex = BitmapIndex;
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT goniometer_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void goniometer_t::DeleteDeviceIndependentResources() noexcept
{
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT goniometer_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_RecreateStyles)
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

        ComPtr<ID2D1DeviceContext> DeviceContext;

        hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, DeviceContext.GetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = DeviceContext.As(&_DeviceContext);

        if (FAILED(hr))
            return hr;
    }

    if (_OpacityEffect == nullptr)
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1Opacity, _OpacityEffect.GetAddressOf());

        if (FAILED(hr))
            return hr;

        constexpr FLOAT Persistence = 120.f; // ms

        const FLOAT Opacity = std::expf(-(1000.f / (FLOAT) _State->_RefreshRateLimit) / Persistence);

        _OpacityEffect->SetValue(D2D1_OPACITY_PROP_OPACITY, Opacity);
    }

    if (_BlurEffect == nullptr)
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &_BlurEffect);

        if (FAILED(hr))
            return hr;

        _BlurEffect->SetInputEffect(0, _OpacityEffect.Get());

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, _State->_BlurSigma);
        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_DIRECTIONALBLUR_OPTIMIZATION_BALANCED);
        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
    }

    if (_Sprite == nullptr)
    {
        hr = CreatePointSprite(_Sprite);

        if (FAILED(hr))
            return hr;
    }

    if (_SpriteBatch == nullptr)
    {
        hr = _DeviceContext->CreateSpriteBatch(&_SpriteBatch);

        if (FAILED(hr))
            return hr;
    }

    // Create a brush stroke style for the axes that remains fixed during the scaling transformation.
    if (_StaticStrokeStyle == nullptr)
    {
        D2D1_STROKE_STYLE_PROPERTIES1 StrokeStyleProperties = D2D1::StrokeStyleProperties1();

        StrokeStyleProperties.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED; // Prevent stroke scaling

        hr = _Direct2D.Factory->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, _StaticStrokeStyle.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    hr = CreateSizeDependentResources(deviceContext);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void goniometer_t::DeleteDeviceSpecificResources() noexcept
{
    DeleteSizeDependentResources();

    _StaticStrokeStyle.Reset();

    _SpriteBatch.Reset();
    _Sprite.Reset();

    _OpacityEffect.Reset();
    _BlurEffect.Reset();

    _DeviceContext.Reset();

#ifdef _DEBUG
    _DebugBrush.Reset();
#endif
}

/// <summary>
/// Creates the resources that depend on the size of the backbuffer.
/// </summary>
HRESULT goniometer_t::CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if ((_Size.width <= 0.f) || _Size.height <= 0.f)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    if (_SignalStyle._Brush == nullptr)
    {
        _SignalStyle = *_State->_StyleManager.GetStyle(VisualElement::SignalLine);

        _SignalStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _SignalStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", _ScaleFactor);

        if (FAILED(hr))
            return hr;

        if (_AudioProcessor.GetColorMode() == goniometer::ColorMode::Mono)
        {
            _SignalStyle._CurrentColor = _SignalStyle._CustomColor;

            _AudioProcessor.SetMonoColor(_SignalStyle._CurrentColor);
        }
        else
        if (_AudioProcessor.GetColorMode() == goniometer::ColorMode::Triband)
        {
            auto cl = _SignalStyle._CurrentGradientStops.front().color;
            auto ch = _SignalStyle._CurrentGradientStops.back().color;

            D2D1_COLOR_F cm = { };

            // Find the first color that is in the middle of the gradient.
            for (auto & gs : _SignalStyle._CurrentGradientStops)
            {
                cm = gs.color;

                if (msc::InRange(gs.position, 1.f / 3.f, 2.f / 3.f))
                    break;
            }

            _AudioProcessor.SetTriBandColors(cl, cm, ch);
        }
    }

    if (_XAxisLineStyle._Brush == nullptr)
    {
        _XAxisLineStyle = *_State->_StyleManager.GetStyle(VisualElement::XAxisLine);

        _XAxisLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        // The font style is created prescaled to counter the Scale transform in the command list.
        hr = _XAxisLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (FAILED(hr))
            return hr;
    }

    if (_XAxisTextStyle._Brush == nullptr)
    {
        _XAxisTextStyle = *_State->_StyleManager.GetStyle(VisualElement::XAxisText);

        _XAxisTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        // The font style is created prescaled to counter the Scale transform in the command list.
        hr = _XAxisTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"L", _ScaleFactor);

        if (FAILED(hr))
            return hr;
    }

    if (_YAxisLineStyle._Brush == nullptr)
    {
        _YAxisLineStyle = *_State->_StyleManager.GetStyle(VisualElement::YAxisLine);

        _YAxisLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _YAxisLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (FAILED(hr))
            return hr;
    }

    if (_YAxisTextStyle._Brush == nullptr)
    {
        _YAxisTextStyle = *_State->_StyleManager.GetStyle(VisualElement::YAxisText);

        _YAxisTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        // The font style is created prescaled to counter the Scale transform in the command list.
        hr = _YAxisTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"R", _ScaleFactor);

        if (FAILED(hr))
            return hr;
    }

    const D2D1_BITMAP_PROPERTIES1 BitmapProperties = D2D1::BitmapProperties1
    (
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED) // Required for alpha transparency. Otherwise use D2D1_ALPHA_MODE_IGNORE.
    );

    if ((_Bitmaps[0] == nullptr) || (_Bitmaps[1] == nullptr))
    {
        const FLOAT x = (_Size.width  - _Side) / 2.f;
        const FLOAT y = (_Size.height - _Side) / 2.f;

        _DestinationRectangle = { x, y, x + _Side, y + _Side };

        hr = deviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Side, (UINT32) _Side), nullptr, 0, &BitmapProperties, _Bitmaps[0].GetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = deviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Side, (UINT32) _Side), nullptr, 0, &BitmapProperties, _Bitmaps[1].GetAddressOf());

        if (FAILED(hr))
            return hr;

        _PrevBitmapIndex = 0;

        hr = ClearBitmaps();

        if (FAILED(hr))
            return hr;
    }

    if (_StaticContent == nullptr)
        hr = CreateStaticContent();

    return hr;
}

/// <summary>
/// Deletes the resources that depend on the size of the backbuffer.
/// </summary>
void goniometer_t::DeleteSizeDependentResources() noexcept
{
    _StaticContent.Reset();

    for (auto & Bitmap : _Bitmaps)
        Bitmap.Reset();

    _YAxisTextStyle.DeleteDeviceSpecificResources();
    _YAxisLineStyle.DeleteDeviceSpecificResources();
    _XAxisTextStyle.DeleteDeviceSpecificResources();
    _XAxisLineStyle.DeleteDeviceSpecificResources();

    _SignalStyle.DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates the sprites from the calculates points.
/// </summary>
HRESULT goniometer_t::CreateSprites() noexcept
{
    HRESULT hr = S_OK;

    if (_SpriteDestinations.size() != _AudioProcessor._PointCount)
    {
        _SpriteDestinations.resize(_AudioProcessor._PointCount);
        _SpriteSources     .resize(_AudioProcessor._PointCount);
        _SpriteColors      .resize(_AudioProcessor._PointCount);
        _SpriteTransforms  .resize(_AudioProcessor._PointCount);

        std::fill(_SpriteSources.begin(),    _SpriteSources.end(),    SpriteRectangle);
        std::fill(_SpriteTransforms.begin(), _SpriteTransforms.end(), D2D1::Matrix3x2F::Identity());
    }

    const auto Side2 = _Side / 2.f;

    for (size_t i = 0; i < _AudioProcessor._PointCount; ++i)
    {
        const auto & Point = _AudioProcessor._Points[i];

        const FLOAT CenterX = Side2 + (Point.x * _ScaleFactor * Radius);
        const FLOAT CenterY = Side2 + (Point.y * _ScaleFactor * Radius);

        _SpriteDestinations[i] = D2D1::RectF(CenterX - SpriteRadius, CenterY - SpriteRadius, CenterX + SpriteRadius, CenterY + SpriteRadius);
        _SpriteColors      [i] = Point.Color;
    }

    const auto SpriteCount = _SpriteBatch->GetSpriteCount();

    if (SpriteCount != _AudioProcessor._PointCount)
    {
        _SpriteBatch->Clear();

        hr = _SpriteBatch->AddSprites
        (
            (UINT32) _SpriteDestinations.size(),
            _SpriteDestinations.data(),
            _SpriteSources.data(),
            _SpriteColors.data(),
            _SpriteTransforms.data(),
            sizeof(D2D1_RECT_F),
            sizeof(D2D1_RECT_U),
            sizeof(D2D1_COLOR_F),
            sizeof(D2D1_MATRIX_3X2_F)
        );
    }
    else
    {
        hr = _SpriteBatch->SetSprites
        (
            0u,
            SpriteCount,
            _SpriteDestinations.data(),
            _SpriteSources.data(),
            _SpriteColors.data(),
            _SpriteTransforms.data(),
            sizeof(D2D1_RECT_F),
            sizeof(D2D1_RECT_U),
            sizeof(D2D1_COLOR_F),
            sizeof(D2D1_MATRIX_3X2_F)
        );
    }

    return hr;
}

/// <summary>
/// Clears the back buffers.
/// </summary>
HRESULT goniometer_t::ClearBitmaps() noexcept
{
    HRESULT hr = E_FAIL;

    _DeviceContext->BeginDraw();

    for (auto Bitmap : _Bitmaps)
    {
        if (Bitmap == nullptr)
            continue;

        _DeviceContext->SetTarget(Bitmap.Get());

        _DeviceContext->Clear(); // Transparent
    }

    _DeviceContext->SetTarget(nullptr);

    hr = _DeviceContext->EndDraw();

    return hr;
}

/// <summary>
/// Creates the point sprite.
/// </summary>
HRESULT goniometer_t::CreatePointSprite(ComPtr<ID2D1Bitmap1> & bitmap) noexcept
{
    if (_DeviceContext == nullptr)
        return E_INVALIDARG;

    bitmap.Reset();

    const D2D1_SIZE_U Size = { (UINT32) (SpriteRadius * 2.f), (UINT32) (SpriteRadius * 2.f) };

    const D2D1_BITMAP_PROPERTIES1 Properties = D2D1::BitmapProperties1
    (
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    HRESULT hr = _DeviceContext->CreateBitmap(Size, nullptr, 0, &Properties, &bitmap);

    if (FAILED(hr))
        return hr;

    _DeviceContext->SetTarget(bitmap.Get());

    _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    _DeviceContext->BeginDraw();

    _DeviceContext->Clear(); // Transparent

    ComPtr<ID2D1SolidColorBrush> WhiteBrush;

    hr = _DeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &WhiteBrush);

    if (SUCCEEDED(hr))
    {
        auto Ellipse = D2D1::Ellipse(D2D1::Point2F(SpriteRadius, SpriteRadius), SpriteRadius, SpriteRadius);

        _DeviceContext->FillEllipse(Ellipse, WhiteBrush.Get());
    }

    hr = _DeviceContext->EndDraw();

    if (FAILED(hr))
        bitmap.Reset();

    return hr;
}

/// <summary>
/// Creates a command list to render static content.
/// This is created in a [-1, 1] axis setup and scaled up as necessary.
/// </summary>
HRESULT goniometer_t::CreateStaticContent() noexcept
{
    const auto ScaleTransform = D2D1::Matrix3x2F::Scale(D2D1::SizeF(_ScaleFactor, _ScaleFactor));

    // Create a command list that will store the grid pattern and the axes.
    HRESULT hr = _DeviceContext->CreateCommandList(&_StaticContent);

    if (FAILED(hr))
        return hr;

    _DeviceContext->SetTarget(_StaticContent.Get());
    _DeviceContext->BeginDraw();

    _DeviceContext->SetTransform(ScaleTransform);

    _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    FLOAT Sin, Cos;

    ::D2D1SinCos(std::numbers::pi_v<FLOAT> / 4.f, &Sin, &Cos);

    // Draw the L-axis.
    {
        _DeviceContext->DrawLine(D2D1::Point2F(-Radius * Sin, -Radius * Cos), D2D1::Point2F(Radius * Sin, Radius * Cos), _XAxisLineStyle._Brush, 1.f, _StaticStrokeStyle.Get());

        _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        _XAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

        const D2D1_RECT_F TextRect = {-Sin, -Cos, -Radius * Sin, -Radius * Cos };

        _DeviceContext->DrawText(L"L", 1u, _XAxisTextStyle._TextFormat, TextRect, _XAxisTextStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    }

    // Draw the R-axis.
    {
        _DeviceContext->DrawLine(D2D1::Point2F(-Radius * Sin, Radius * Cos), D2D1::Point2F(Radius * Sin, -Radius * Cos), _YAxisLineStyle._Brush, 1.f, _StaticStrokeStyle.Get());

        _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        _XAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

        const D2D1_RECT_F TextRect = { Radius * Sin, -Radius * Cos, Sin, -Cos };

        _DeviceContext->DrawText(L"R", 1u, _XAxisTextStyle._TextFormat, TextRect, _XAxisTextStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    }

    // Draw the S-axis.
    {
        _DeviceContext->DrawLine(D2D1::Point2F(-Radius, 0.f), D2D1::Point2F(Radius, 0.f), _XAxisLineStyle._Brush, 1.f, _StaticStrokeStyle.Get());

        {
            _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            _XAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            const D2D1_RECT_F TextRect = { -Radius, 0.f, -Radius, 0.f };

            _DeviceContext->DrawText(L"+S", 2u, _XAxisTextStyle._TextFormat, TextRect, _XAxisTextStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
        }

        {
            _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

            const D2D1_RECT_F TextRect = { Radius, 0.f, Radius, 0.f };

            _DeviceContext->DrawText(L"-S", 2u, _XAxisTextStyle._TextFormat, TextRect, _XAxisTextStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
    }

    // Draw the M-axis.
    {
        _DeviceContext->DrawLine(D2D1::Point2F(0.f, -Radius), D2D1::Point2F(0, Radius), _XAxisLineStyle._Brush, 1.f, _StaticStrokeStyle.Get());
/*
        _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        _XAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        const D2D1_RECT_F TextRect = {-r, 0.f, r, 0.f };

        _DeviceContext->DrawText(L"L", 1u, _XAxisTextStyle._TextFormat, TextRect, _XAxisTextStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
*/
    }

    // Draw the outer circle (0 dBFS).
    {
        auto Ellipse = D2D1::Ellipse(D2D1::Point2F(0.f, 0.f), Radius, Radius);

        _DeviceContext->DrawEllipse(Ellipse, _XAxisLineStyle._Brush, 1.f, _StaticStrokeStyle.Get());
    }

    // Draw the middle circle (-6 dBFS).
    {
        const auto r = Radius * std::powf(10.f, -6.f / 20.f);

        auto Ellipse = D2D1::Ellipse(D2D1::Point2F(0.f, 0.f), r, r);

        _DeviceContext->DrawEllipse(Ellipse, _XAxisLineStyle._Brush, 1.f, _StaticStrokeStyle.Get());
    }

    // Draw the inner circle (-12 dBFS).
    {
        const auto r = Radius * std::powf(10.f, -12.f / 20.f);

        auto Ellipse = D2D1::Ellipse(D2D1::Point2F(0.f, 0.f), r, r);


        _DeviceContext->DrawEllipse(Ellipse, _XAxisLineStyle._Brush, 1.f, _StaticStrokeStyle.Get());
    }

    _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

    hr = _DeviceContext->EndDraw();

    if (FAILED(hr))
        return hr;

    hr = _StaticContent->Close();

    return hr;
}

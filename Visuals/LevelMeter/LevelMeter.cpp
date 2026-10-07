
/** $VER: LevelMeter.cpp (2026.10.07) P. Stuer - Implements a left/right/mid/side level meter. **/

#include "pch.h"

#include "LevelMeter.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
level_meter_t::level_meter_t()
{
    _Rect = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
level_meter_t::~level_meter_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void level_meter_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void level_meter_t::Move(const D2D1_RECT_F & rect) noexcept
{
    InitializeMetrics(rect);
}

/// <summary>
/// Renders this instance.
/// </summary>
void level_meter_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    const D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_Rect.left, _Rect.top);

    deviceContext->SetTransform(Translate);

    const FLOAT w = (_Rect.right  - _Rect.left);
    const FLOAT h = (_Rect.bottom - _Rect.top);

    const FLOAT CenterX = w / 2.f;
    const FLOAT CenterY = h / 2.f;

    const FLOAT LEDHeight = _State->_LEDLight + _State->_LEDGap;

    ID2D1Bitmap * OpacityMask = _OpacityMask.Get();

    const auto RenderHorizontalBar = [this, deviceContext, LEDHeight, OpacityMask](D2D1_RECT_F & Rect, auto & Style) noexcept
    {
        if (!Style.IsEnabled())
            return;

        if (_State->_LEDMode)
        {
            if (_State->_LEDIntegralSize && (LEDHeight > 0.f))
                Rect.right = std::ceil(Rect.right / LEDHeight) * LEDHeight;

            deviceContext->FillOpacityMask(OpacityMask, Style._Brush.Get(), D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
        }
        else
            deviceContext->FillRectangle(Rect, Style._Brush.Get());
    };

    const auto RenderVerticalBar = [this, deviceContext, LEDHeight, OpacityMask](D2D1_RECT_F & Rect, auto & Style) noexcept
    {
        if (!Style.IsEnabled())
            return;

        if (_State->_LEDMode)
        {
            if (_State->_LEDIntegralSize && (LEDHeight > 0.f))
                Rect.bottom = std::ceil(Rect.bottom / LEDHeight) * LEDHeight;

            deviceContext->FillOpacityMask(OpacityMask, Style._Brush.Get(), D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
        }
        else
            deviceContext->FillRectangle(Rect, Style._Brush.Get());
    };

    if (_State->_IsHorizontalLevelMeter)
    {
        // Render the bars.
        {
            auto x = (FLOAT) _Analysis->_Balance * w;

            D2D1_RECT_F Rect = { CenterX, 2.f, x, CenterY - 2.f };

            RenderHorizontalBar(Rect, _LeftRightStyle);

            if (_LeftRightIndicatorStyle.IsEnabled())
            {
                Rect.left  = x - _LeftRightIndicatorStyle._Thickness;
                Rect.right = x + _LeftRightIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _LeftRightIndicatorStyle._Brush.Get());
            }

            x = (FLOAT) _Analysis->_Phase * w;

            Rect = { CenterX, CenterY + 2.f, x, h - 2.f };

            RenderHorizontalBar(Rect, _MidSideStyle);

            if (_MidSideIndicatorStyle.IsEnabled())
            {
                Rect.left  = x - _MidSideIndicatorStyle._Thickness;
                Rect.right = x + _MidSideIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _MidSideIndicatorStyle._Brush.Get());
            }
        }

        // Render the axis.
        if (_AxisStyle.IsEnabled())
        {
            ID2D1Brush * Brush = _AxisStyle._Brush.Get();
            IDWriteTextFormat * TextFormat = _AxisStyle._TextFormat.Get();

            deviceContext->DrawLine({ 2.f, CenterY }, { w - 2.f, CenterY }, Brush, _AxisStyle._Thickness);

            D2D1_RECT_F Rect = { 4.f, 2.f, w - 4.f, CenterY - 2.f };

            {
                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                deviceContext->DrawText(L"L", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                deviceContext->DrawText(L"R", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.top    = CenterY + 2.f;
                Rect.bottom = h       - 2.f;

                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                deviceContext->DrawText(L"S", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                deviceContext->DrawText(L"M", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, h - 2.f }, Brush, _AxisStyle._Thickness);
        }
    }
    else
    {
        // Render the bars.
        {
            auto y = (FLOAT) _Analysis->_Balance * h;

            D2D1_RECT_F Rect = { 2.f, CenterY, CenterX - 2.f, y };

            RenderVerticalBar(Rect, _LeftRightStyle);

            if (_LeftRightIndicatorStyle.IsEnabled())
            {
                Rect.top    = y - _LeftRightIndicatorStyle._Thickness;
                Rect.bottom = y + _LeftRightIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _LeftRightIndicatorStyle._Brush.Get());
            }

            y = (FLOAT) _Analysis->_Phase * h;

            Rect = { CenterX + 2.f, CenterY, w - 2.f, y };

            RenderVerticalBar(Rect, _MidSideStyle);

            if (_MidSideIndicatorStyle.IsEnabled())
            {
                Rect.top    = y - _MidSideIndicatorStyle._Thickness;
                Rect.bottom = y + _MidSideIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _MidSideIndicatorStyle._Brush.Get());
            }
        }

        // Render the axis.
        if (_AxisStyle.IsEnabled())
        {
            ID2D1Brush * Brush = _AxisStyle._Brush.Get();
            IDWriteTextFormat * TextFormat = _AxisStyle._TextFormat.Get();

            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, h - 2.f }, Brush, _AxisStyle._Thickness);

            D2D1_RECT_F Rect = { 2.f, 4.f, CenterX - 2.f, h - 4.f };

            {
                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                deviceContext->DrawText(L"L", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                deviceContext->DrawText(L"R", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.left  = CenterX + 2.f;
                Rect.right = w       - 2.f;

                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                deviceContext->DrawText(L"S", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                deviceContext->DrawText(L"M", 1, TextFormat, Rect, Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, h - 2.f }, Brush, _AxisStyle._Thickness);
        }
    }

    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT level_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_ResizeResources)
        DeleteDeviceSpecificResources();

    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _DebugBrush.GetAddressOf());
#endif

//  D2D1_SIZE_F Size = deviceContext->GetSize();

    if (_OpacityMask == nullptr)
    {
        hr = CreateOpacityMask(deviceContext);

        if (FAILED(hr))
            return hr;
    }

    const auto InitializeStyle = [this, deviceContext](auto & style, VisualElement visualElement, const wchar_t * text) noexcept -> HRESULT
    {
        if (style._Brush != nullptr)
            return S_OK;

        style = *_State->_StyleManager.GetStyle(visualElement);

        style.SetColor(_State);

        return style.CreateDeviceSpecificResources(deviceContext, _Size, text, 1.f);
    };

    hr = InitializeStyle(_LeftRightStyle, VisualElement::BarLeftRight, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_LeftRightIndicatorStyle, VisualElement::BarLeftRightIndicator, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_MidSideStyle, VisualElement::BarMidSide, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_MidSideIndicatorStyle, VisualElement::BarMidSideIndicator, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_AxisStyle, VisualElement::LevelMeterAxis, L"+1.0");

    if (FAILED(hr))
        return hr;

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void level_meter_t::DeleteDeviceSpecificResources() noexcept
{
    _AxisStyle.DeleteDeviceSpecificResources();
    _MidSideIndicatorStyle.DeleteDeviceSpecificResources();
    _MidSideStyle.DeleteDeviceSpecificResources();
    _LeftRightIndicatorStyle.DeleteDeviceSpecificResources();
    _LeftRightStyle.DeleteDeviceSpecificResources();

    _OpacityMask.Reset();

#ifdef _DEBUG
    _DebugBrush.Reset();
#endif
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT level_meter_t::CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept
{
    D2D1_SIZE_F Size = deviceContext->GetSize();

    ComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = deviceContext->CreateCompatibleRenderTarget(Size, rt.GetAddressOf());

    if (FAILED(hr))
        return hr;

    rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    ComPtr<ID2D1SolidColorBrush> Brush;

    hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), Brush.GetAddressOf()); // Black parts will be masked out.

    if (FAILED(hr))
        return hr;

    rt->BeginDraw();

    rt->Clear(); // Transparent

    const FLOAT LEDSize = _State->_LEDLight + _State->_LEDGap;

    if (LEDSize > 0.f)
    {
        if (_State->_IsHorizontalLevelMeter)
        {
            FLOAT w = Size.width;

            if (_State->_LEDIntegralSize)
                w = std::ceil(w / LEDSize) * LEDSize;

            for (FLOAT x = ((Size.width - w) / 2.f) + _State->_LEDGap; x < w; x += LEDSize)
                rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDLight, Size.height), Brush.Get());
        }
        else
        {
            FLOAT h = Size.height;

            if (_State->_LEDIntegralSize)
                h = std::ceil(h / LEDSize) * LEDSize;

            for (FLOAT y = ((Size.height - h) / 2.f) + _State->_LEDGap; y < h; y += LEDSize)
                rt->FillRectangle(D2D1::RectF(0.f, y, Size.width, y + _State->_LEDLight), Brush.Get());
        }
    }

    hr = rt->EndDraw();

    if (FAILED(hr))
        return hr;

    hr = rt->GetBitmap(_OpacityMask.GetAddressOf());

    return hr;
}

/// <summary>
/// Handles a configuration change.
/// </summary>
void level_meter_t::OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept
{
    if (!IsSet(configurationChanges, ConfigurationChanges::Layout))
        return;

    _State->_ResizeResources = true;
}

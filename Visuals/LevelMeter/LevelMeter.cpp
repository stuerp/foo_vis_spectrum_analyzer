
/** $VER: LevelMeter.cpp (2026.06.17) P. Stuer - Implements a left/right/mid/side level meter. **/

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
void level_meter_t::Initialize(state_t * state, graph_options_t * graphOptions, const analysis_t * analysis, bool isFirst, bool isLast) noexcept
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
    SetRect(rect);
}

/// <summary>
/// Renders this instance.
/// </summary>
void level_meter_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    const D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_Rect.left, _Rect.top);

    deviceContext->SetTransform(Translate);

    const FLOAT w = (_Rect.right  - _Rect.left);
    const FLOAT h = (_Rect.bottom - _Rect.top);

    const FLOAT CenterX = w / 2.f;
    const FLOAT CenterY = h / 2.f;

    const FLOAT LEDHeight = _State->_LEDLight + _State->_LEDGap;

    if (_State->_IsHorizontalLevelMeter)
    {
        // Render the bars.
        {
            FLOAT x = (FLOAT) _Analysis->_Balance * w;

            D2D1_RECT_F Rect = { CenterX, 2.f, x, CenterY - 2.f };

            if (_LeftRightStyle.IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _LeftRightStyle._Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.right = std::ceil(Rect.right / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _LeftRightStyle._Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_LeftRightIndicatorStyle.IsEnabled())
            {
                Rect.left  = x - _LeftRightIndicatorStyle._Thickness;
                Rect.right = x + _LeftRightIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _LeftRightIndicatorStyle._Brush);
            }

            x = (FLOAT) _Analysis->_Phase * w;

            Rect = { CenterX, CenterY + 2.f, x, h - 2.f };

            if (_MidSideStyle.IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _MidSideStyle._Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.right = std::ceil(Rect.right / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _MidSideStyle._Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_MidSideIndicatorStyle.IsEnabled())
            {
                Rect.left  = x - _MidSideIndicatorStyle._Thickness;
                Rect.right = x + _MidSideIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _MidSideIndicatorStyle._Brush);
            }
        }

        // Render the axis.
        if (_AxisStyle.IsEnabled())
        {
            deviceContext->DrawLine({ 2.f, CenterY }, { w - 2.f, CenterY }, _AxisStyle._Brush, _AxisStyle._Thickness);

            D2D1_RECT_F Rect = { 4.f, 2.f, w - 4.f, CenterY - 2.f };

            {
                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                deviceContext->DrawText(L"L", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                deviceContext->DrawText(L"R", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.top    = CenterY + 2.f;
                Rect.bottom = h       - 2.f;

                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                deviceContext->DrawText(L"S", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                deviceContext->DrawText(L"M", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, h - 2.f }, _AxisStyle._Brush, _AxisStyle._Thickness);
        }
    }
    else
    {
        // Render the bars.
        {
            FLOAT y = (FLOAT) _Analysis->_Balance * h;

            D2D1_RECT_F Rect = { 2.f, CenterY, CenterX - 2.f, y };

            if (_LeftRightStyle.IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _LeftRightStyle._Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.bottom = std::ceil(Rect.bottom / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _LeftRightStyle._Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_LeftRightIndicatorStyle.IsEnabled())
            {
                Rect.top    = y - _LeftRightIndicatorStyle._Thickness;
                Rect.bottom = y + _LeftRightIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _LeftRightIndicatorStyle._Brush);
            }

            y = (FLOAT) _Analysis->_Phase * h;

            Rect = { CenterX + 2.f, CenterY, w - 2.f, y };

            if (_MidSideStyle.IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _MidSideStyle._Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.bottom = std::ceil(Rect.bottom / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _MidSideStyle._Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_MidSideIndicatorStyle.IsEnabled())
            {
                Rect.top    = y - _MidSideIndicatorStyle._Thickness;
                Rect.bottom = y + _MidSideIndicatorStyle._Thickness;

                deviceContext->FillRectangle(Rect, _MidSideIndicatorStyle._Brush);
            }
        }

        // Render the axis.
        if (_AxisStyle.IsEnabled())
        {
            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, h - 2.f }, _AxisStyle._Brush, _AxisStyle._Thickness);

            D2D1_RECT_F Rect = { 2.f, 4.f, CenterX - 2.f, h - 4.f };

            {
                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                deviceContext->DrawText(L"L", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                deviceContext->DrawText(L"R", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.left  = CenterX + 2.f;
                Rect.right = w       - 2.f;

                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                deviceContext->DrawText(L"S", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                deviceContext->DrawText(L"M", 1, _AxisStyle._TextFormat, Rect, _AxisStyle._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, h - 2.f }, _AxisStyle._Brush, _AxisStyle._Thickness);
        }
    }

    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT level_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = deviceContext->GetSize();

    if (_OpacityMask == nullptr)
    {
        hr = CreateOpacityMask(deviceContext);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_LeftRightStyle._Brush == nullptr)
    {
        _LeftRightStyle = *_State->_StyleManager.GetStyle(VisualElement::BarLeftRight);

        _LeftRightStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _LeftRightStyle.CreateDeviceSpecificResources(deviceContext, Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_LeftRightIndicatorStyle._Brush == nullptr)
    {
        _LeftRightIndicatorStyle = *_State->_StyleManager.GetStyle(VisualElement::BarLeftRightIndicator);

        _LeftRightIndicatorStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _LeftRightIndicatorStyle.CreateDeviceSpecificResources(deviceContext, Size, L"", 1.f);
    }

    if (!SUCCEEDED(hr))
        return hr;

    if (_MidSideStyle._Brush == nullptr)
    {
        _MidSideStyle = *_State->_StyleManager.GetStyle(VisualElement::BarMidSide);

        _MidSideStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _MidSideStyle.CreateDeviceSpecificResources(deviceContext, Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_MidSideIndicatorStyle._Brush == nullptr)
    {
        _MidSideIndicatorStyle = *_State->_StyleManager.GetStyle(VisualElement::BarMidSideIndicator);

        _MidSideIndicatorStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _MidSideIndicatorStyle.CreateDeviceSpecificResources(deviceContext, Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_AxisStyle._Brush == nullptr)
    {
        _AxisStyle = *_State->_StyleManager.GetStyle(VisualElement::LevelMeterAxis);

        _AxisStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _AxisStyle.CreateDeviceSpecificResources(deviceContext, Size, L"+1.0", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
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
void level_meter_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    _AxisStyle.DeleteDeviceSpecificResources();
    _MidSideIndicatorStyle.DeleteDeviceSpecificResources();
    _MidSideStyle.DeleteDeviceSpecificResources();
    _LeftRightIndicatorStyle.DeleteDeviceSpecificResources();
    _LeftRightStyle.DeleteDeviceSpecificResources();

    _OpacityMask.Release();
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT level_meter_t::CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept
{
    D2D1_SIZE_F Size = deviceContext->GetSize();

    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = deviceContext->CreateCompatibleRenderTarget(D2D1::SizeF(Size.width, Size.height), &rt);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 1.f), &Brush);

        if (SUCCEEDED(hr))
        {
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
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDLight, Size.height), Brush);
                }
                else
                {
                    FLOAT h = Size.height;

                    if (_State->_LEDIntegralSize)
                        h = std::ceil(h / LEDSize) * LEDSize;

                    for (FLOAT y = ((Size.height - h) / 2.f) + _State->_LEDGap; y < h; y += LEDSize)
                        rt->FillRectangle(D2D1::RectF(0.f, y, Size.width, y + _State->_LEDLight), Brush);
                }
            }

            hr = rt->EndDraw();
        }

        if (SUCCEEDED(hr))
            hr = rt->GetBitmap(&_OpacityMask);
    }

    return hr;
}

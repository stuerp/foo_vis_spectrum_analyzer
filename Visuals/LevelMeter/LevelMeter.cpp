
/** $VER: LevelMeter.cpp (2025.10.08) P. Stuer - Implements a left/right/mid/side level meter. **/

#include "pch.h"

#include "LevelMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
level_meter_t::level_meter_t()
{
    _Bounds = { };
    _Size = { };

    _LeftRightStyle =
    _MidSideStyle =
    _AxisStyle =  nullptr;

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
level_meter_t::~level_meter_t()
{
    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void level_meter_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void level_meter_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void level_meter_t::Reset() noexcept
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void level_meter_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = false;
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

    const FLOAT CenterX = GetWidth()  / 2.f;
    const FLOAT CenterY = GetHeight() / 2.f;

    const FLOAT LEDHeight = _State->_LEDSize + _State->_LEDGap;

    if (_State->_HorizontalLevelMeter)
    {
        // Render the gauges.
        {
            FLOAT x = (FLOAT) _Analysis->_Balance * GetWidth();

            D2D1_RECT_F Rect = { CenterX, 2.f, x, CenterY - 2.f };

            if (_LeftRightStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _LeftRightStyle->_Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.right = std::ceil(Rect.right / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _LeftRightStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_LeftRightIndicatorStyle->IsEnabled())
            {
                Rect.left  = x - _LeftRightIndicatorStyle->_Thickness;
                Rect.right = x + _LeftRightIndicatorStyle->_Thickness;

                deviceContext->FillRectangle(Rect, _LeftRightIndicatorStyle->_Brush);
            }

            x = (FLOAT) _Analysis->_Phase * GetWidth();

            Rect = { CenterX, CenterY + 2.f, x, GetHeight() - 2.f };

            if (_MidSideStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _MidSideStyle->_Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.right = std::ceil(Rect.right / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _MidSideStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_MidSideIndicatorStyle->IsEnabled())
            {
                Rect.left  = x - _MidSideIndicatorStyle->_Thickness;
                Rect.right = x + _MidSideIndicatorStyle->_Thickness;

                deviceContext->FillRectangle(Rect, _MidSideIndicatorStyle->_Brush);
            }
        }

        // Render the axis.
        if (_AxisStyle->IsEnabled())
        {
            deviceContext->DrawLine({ 2.f, CenterY }, { GetWidth() - 2.f, CenterY }, _AxisStyle->_Brush, _AxisStyle->_Thickness);

            D2D1_RECT_F Rect = { 4.f, 2.f, GetWidth() - 4.f, CenterY - 2.f };

            {
                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                deviceContext->DrawText(L"L", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                deviceContext->DrawText(L"R", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.top    = CenterY     + 2.f;
                Rect.bottom = GetHeight() - 2.f;

                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                deviceContext->DrawText(L"S", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                deviceContext->DrawText(L"M", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, GetHeight() - 2.f }, _AxisStyle->_Brush, _AxisStyle->_Thickness);
        }
    }
    else
    {
        // Render the gauges.
        {
            FLOAT y = (FLOAT) _Analysis->_Balance * GetHeight();

            D2D1_RECT_F Rect = { 2.f, CenterY, CenterX - 2.f, y };

            if (_LeftRightStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _LeftRightStyle->_Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.bottom = std::ceil(Rect.bottom / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _LeftRightStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_LeftRightIndicatorStyle->IsEnabled())
            {
                Rect.top    = y - _LeftRightIndicatorStyle->_Thickness;
                Rect.bottom = y + _LeftRightIndicatorStyle->_Thickness;

                deviceContext->FillRectangle(Rect, _LeftRightIndicatorStyle->_Brush);
            }

            y = (FLOAT) _Analysis->_Phase * GetHeight();

            Rect = { CenterX + 2.f, CenterY, GetWidth() - 2.f, y };

            if (_MidSideStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(Rect, _MidSideStyle->_Brush);
                else
                {
                    if (_State->_LEDIntegralSize)
                        Rect.bottom = std::ceil(Rect.bottom / LEDHeight) * LEDHeight;

                    deviceContext->FillOpacityMask(_OpacityMask, _MidSideStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }
            }

            if (_MidSideIndicatorStyle->IsEnabled())
            {
                Rect.top    = y - _MidSideIndicatorStyle->_Thickness;
                Rect.bottom = y + _MidSideIndicatorStyle->_Thickness;

                deviceContext->FillRectangle(Rect, _MidSideIndicatorStyle->_Brush);
            }
        }

        // Render the axis.
        if (_AxisStyle->IsEnabled())
        {
            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, GetHeight() - 2.f }, _AxisStyle->_Brush, _AxisStyle->_Thickness);

            D2D1_RECT_F Rect = { 2.f, 4.f, CenterX - 2.f, GetHeight() - 4.f };

            {
                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                deviceContext->DrawText(L"L", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                deviceContext->DrawText(L"R", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.left  = CenterX    + 2.f;
                Rect.right = GetWidth() - 2.f;

                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                deviceContext->DrawText(L"S", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                deviceContext->DrawText(L"M", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            deviceContext->DrawLine({ CenterX, 2.f }, { CenterX, GetHeight() - 2.f }, _AxisStyle->_Brush, _AxisStyle->_Thickness);
        }
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT level_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = deviceContext->GetSize();

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(deviceContext);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeLeftRight, deviceContext, Size, L"", 1.f, &_LeftRightStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeLeftRightIndicator, deviceContext, Size, L"", 1.f, &_LeftRightIndicatorStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeMidSide, deviceContext, Size, L"", 1.f, &_MidSideStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeMidSideIndicator, deviceContext, Size, L"", 1.f, &_MidSideIndicatorStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::LevelMeterAxis, deviceContext, Size, L"+1.0", 1.f, &_AxisStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void level_meter_t::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_AxisStyle)
    {
        _AxisStyle->DeleteDeviceSpecificResources();
        _AxisStyle = nullptr;
    }

    if (_MidSideIndicatorStyle)
    {
        _MidSideIndicatorStyle->DeleteDeviceSpecificResources();
        _MidSideIndicatorStyle = nullptr;
    }

    if (_MidSideStyle)
    {
        _MidSideStyle->DeleteDeviceSpecificResources();
        _MidSideStyle = nullptr;
    }

    if (_LeftRightIndicatorStyle)
    {
        _LeftRightIndicatorStyle->DeleteDeviceSpecificResources();
        _LeftRightIndicatorStyle = nullptr;
    }

    if (_LeftRightStyle)
    {
        _LeftRightStyle->DeleteDeviceSpecificResources();
        _LeftRightStyle = nullptr;
    }

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

            rt->Clear();

            const FLOAT LEDSize = _State->_LEDSize + _State->_LEDGap;

            if (LEDSize > 0.f)
            {
                if (_State->_HorizontalLevelMeter)
                {
                    FLOAT w = Size.width;

                    if (_State->_LEDIntegralSize)
                        w = std::ceil(w / LEDSize) * LEDSize;

                    for (FLOAT x = ((Size.width - w) / 2.f) + _State->_LEDGap; x < w; x += LEDSize)
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDSize, Size.height), Brush);
                }
                else
                {
                    FLOAT h = Size.height;

                    if (_State->_LEDIntegralSize)
                        h = std::ceil(h / LEDSize) * LEDSize;

                    for (FLOAT y = ((Size.height - h) / 2.f) + _State->_LEDGap; y < h; y += LEDSize)
                        rt->FillRectangle(D2D1::RectF(0.f, y, Size.width, y + _State->_LEDSize), Brush);
                }
            }

            hr = rt->EndDraw();
        }

        if (SUCCEEDED(hr))
            hr = rt->GetBitmap(&_OpacityMask);
    }

    return hr;
}

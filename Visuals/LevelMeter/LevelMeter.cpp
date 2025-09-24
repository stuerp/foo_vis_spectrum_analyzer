
/** $VER: LevelMeter.cpp (2025.09.24) P. Stuer - Implements a left/right/mid/side level meter. **/

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
void level_meter_t::Render(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    const FLOAT CenterX = GetWidth()  / 2.f;
    const FLOAT CenterY = GetHeight() / 2.f;

    if (_State->_HorizontalLevelMeter)
    {
        // Render the gauges.
        {
            FLOAT x = (FLOAT) _Analysis->_Balance * GetWidth();

            D2D1_RECT_F Rect = { CenterX, 2.f, x, CenterY - 2.f };

            if (_LeftRightStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _LeftRightStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _LeftRightStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            if (_LeftRightIndicatorStyle->IsEnabled())
            {
                Rect.left  = x - _LeftRightIndicatorStyle->_Thickness;
                Rect.right = x + _LeftRightIndicatorStyle->_Thickness;

                renderTarget->FillRectangle(Rect, _LeftRightIndicatorStyle->_Brush);
            }

            x = (FLOAT) _Analysis->_Phase * GetWidth();

            Rect = { CenterX, CenterY + 2.f, x, GetHeight() - 2.f };

            if (_MidSideStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _MidSideStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _MidSideStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            if (_MidSideIndicatorStyle->IsEnabled())
            {
                Rect.left  = x - _MidSideIndicatorStyle->_Thickness;
                Rect.right = x + _MidSideIndicatorStyle->_Thickness;

                renderTarget->FillRectangle(Rect, _MidSideIndicatorStyle->_Brush);
            }
        }

        // Render the axis.
        if (_AxisStyle->IsEnabled())
        {
            renderTarget->DrawLine({ 2.f, CenterY }, { GetWidth() - 2.f, CenterY }, _AxisStyle->_Brush, _AxisStyle->_Thickness);

            D2D1_RECT_F Rect = { 4.f, 2.f, GetWidth() - 4.f, CenterY - 2.f };

            {
                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                renderTarget->DrawText(L"L", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                renderTarget->DrawText(L"R", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.top    = CenterY     + 2.f;
                Rect.bottom = GetHeight() - 2.f;

                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                renderTarget->DrawText(L"S", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                renderTarget->DrawText(L"M", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            renderTarget->DrawLine({ CenterX, 2.f }, { CenterX, GetHeight() - 2.f }, _AxisStyle->_Brush, _AxisStyle->_Thickness);
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
                    renderTarget->FillRectangle(Rect, _LeftRightStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _LeftRightStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            if (_LeftRightIndicatorStyle->IsEnabled())
            {
                Rect.top    = y - _LeftRightIndicatorStyle->_Thickness;
                Rect.bottom = y + _LeftRightIndicatorStyle->_Thickness;

                renderTarget->FillRectangle(Rect, _LeftRightIndicatorStyle->_Brush);
            }

            y = (FLOAT) _Analysis->_Phase * GetHeight();

            Rect = { CenterX + 2.f, CenterY, GetWidth() - 2.f, y };

            if (_MidSideStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _MidSideStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _MidSideStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            if (_MidSideIndicatorStyle->IsEnabled())
            {
                Rect.top    = y - _MidSideIndicatorStyle->_Thickness;
                Rect.bottom = y + _MidSideIndicatorStyle->_Thickness;

                renderTarget->FillRectangle(Rect, _MidSideIndicatorStyle->_Brush);
            }
        }

        // Render the axis.
        if (_AxisStyle->IsEnabled())
        {
            renderTarget->DrawLine({ CenterX, 2.f }, { CenterX, GetHeight() - 2.f }, _AxisStyle->_Brush, _AxisStyle->_Thickness);

            D2D1_RECT_F Rect = { 2.f, 4.f, CenterX - 2.f, GetHeight() - 4.f };

            {
                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                renderTarget->DrawText(L"L", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                renderTarget->DrawText(L"R", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            {
                Rect.left  = CenterX    + 2.f;
                Rect.right = GetWidth() - 2.f;

                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                renderTarget->DrawText(L"S", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                _AxisStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                renderTarget->DrawText(L"M", 1, _AxisStyle->_TextFormat, Rect, _AxisStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            renderTarget->DrawLine({ CenterX, 2.f }, { CenterX, GetHeight() - 2.f }, _AxisStyle->_Brush, _AxisStyle->_Thickness);
        }
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT level_meter_t::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(renderTarget);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeLeftRight, renderTarget, Size, L"", &_LeftRightStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeLeftRightIndicator, renderTarget, Size, L"", &_LeftRightIndicatorStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeMidSide, renderTarget, Size, L"", &_MidSideStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeMidSideIndicator, renderTarget, Size, L"", &_MidSideIndicatorStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::LevelMeterAxis, renderTarget, Size, L"+1.0", &_AxisStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
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
        _AxisStyle->ReleaseDeviceSpecificResources();
        _AxisStyle = nullptr;
    }

    if (_MidSideIndicatorStyle)
    {
        _MidSideIndicatorStyle->ReleaseDeviceSpecificResources();
        _MidSideIndicatorStyle = nullptr;
    }

    if (_MidSideStyle)
    {
        _MidSideStyle->ReleaseDeviceSpecificResources();
        _MidSideStyle = nullptr;
    }

    if (_LeftRightIndicatorStyle)
    {
        _LeftRightIndicatorStyle->ReleaseDeviceSpecificResources();
        _LeftRightIndicatorStyle = nullptr;
    }

    if (_LeftRightStyle)
    {
        _LeftRightStyle->ReleaseDeviceSpecificResources();
        _LeftRightStyle = nullptr;
    }

    _OpacityMask.Release();
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT level_meter_t::CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept
{
    D2D1_SIZE_F Size = renderTarget->GetSize();

    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(Size.width, Size.height), &rt);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 1.f), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->Clear();

            if ((_State->_LEDSize + _State->_LEDGap) > 0.f)
            {
                if (_State->_HorizontalLevelMeter)
                {
                    for (FLOAT x = _State->_LEDGap; x < Size.width; x += (_State->_LEDSize + _State->_LEDGap))
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDSize, Size.height), Brush);
                }
                else
                {
                    for (FLOAT y = _State->_LEDGap; y < Size.height; y += (_State->_LEDSize + _State->_LEDGap))
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


/** $VER: Gauges.cpp (2025.11.02) P. Stuer - Implements the gauges of the peak meter. **/

#include "pch.h"

#include "Gauges.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void gauge_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphDescription = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void gauge_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void gauge_t::Reset() noexcept
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void gauge_t::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    DeleteDeviceSpecificResources();

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void gauge_t::Render(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    if (_Analysis->_GaugeValues.empty() || (GetWidth() <= 0.f) || (GetHeight() <= 0.f))
        return;

    const FLOAT PeakThickness = _MaxPeakStyle->_Thickness / 2.f;

    const FLOAT LEDSize = _State->_LEDSize + _State->_LEDGap;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

#ifdef _DEBUG
    deviceContext->FillRectangle({ 0.f, 0.f, 16.f, 16.f }, _DebugBrush); // Top/Left indicator
#endif

    rect_t Rect = {  };

    if (_State->_HorizontalPeakMeter)
    {
        const FLOAT n = std::ceilf(GetWidth() / LEDSize);
        const FLOAT dx = !_GraphDescription->_FlipHorizontally ? GetWidth() - (n * LEDSize) : (n * LEDSize) - GetWidth();

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.x2 = GetWidth();
            Rect.y2 = std::clamp(Rect.y1 + gaugeMetrics._BarHeight, 0.f, GetHeight());

            const rect_t FullRect = Rect;

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(FullRect, _BackgroundStyle->_Brush);
                else
                    deviceContext->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);
            }

            // Draw the peak.
            {
                // Draw the foreground (Peak).
                if (_PeakStyle->IsEnabled())
                {
                    double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, gaugeMetrics._dBFSZero) : Value.PeakRender;

                    Rect.x2 = (FLOAT) (PeakRender * GetWidth()) + dx;

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _PeakStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                            Rect.x2 = std::ceilf(Rect.x2 / LEDSize) * LEDSize;

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }
                }

                // Draw the foreground (Peak, 0dBFS)
                if ((Value.PeakRender > gaugeMetrics._dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.x1 = (FLOAT) (Value.PeakRender * GetWidth());
                    Rect.x2 = (FLOAT) (gaugeMetrics._dBFSZero * GetWidth());

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                        {
                            Rect.x1 = std::ceilf(Rect.x1 / LEDSize) * LEDSize;
                            Rect.x2 = std::ceilf(Rect.x2 / LEDSize) * LEDSize;
                        }

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }

                    Rect.x1 = FullRect.x1;
                }

                // Draw the foreground (Peak Top).
                if ((_State->_PeakMode != PeakMode::None) && (Value.MaxPeakRender > 0.) && _MaxPeakStyle->IsEnabled())
                {
                    Rect.x1 =
                    Rect.x2 = (FLOAT) (Value.MaxPeakRender * GetWidth());

                    Rect.x1 = std::ceilf(std::clamp(Rect.x1 - PeakThickness, 0.f, GetWidth()));
                    Rect.x2 = std::ceilf(std::clamp(Rect.x2 + PeakThickness, 0.f, GetWidth()));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) Value.Opacity : _MaxPeakStyle->_Opacity;

                    _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                    deviceContext->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                    Rect.x1 = FullRect.x1;
                }
            }

            // Draw the RMS.
            {
                // Draw the foreground (RMS).
                if (_RMSStyle->IsEnabled())
                {
                    double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, gaugeMetrics._dBFSZero) : Value.RMSRender;

                    Rect.x2 = (FLOAT) (RMSRender * GetWidth()) + dx;

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _RMSStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                            Rect.x2 = (std::ceilf(Rect.x2 / LEDSize) + (!_GraphDescription->_FlipHorizontally ? 1.f : -1.f)) * LEDSize;

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }
                }

                // Draw the foreground (RMS, 0dBFS)
                if ((Value.RMSRender > gaugeMetrics._dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.x1 = (FLOAT) (gaugeMetrics._dBFSZero * GetWidth());
                    Rect.x2 = (FLOAT) (Value.RMSRender * GetWidth());

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                        {
                            Rect.x1 = (std::ceilf(Rect.x1 / LEDSize) + (!_GraphDescription->_FlipHorizontally ? 1.f : -1.f)) * LEDSize;
                            Rect.x2 = (std::ceilf(Rect.x2 / LEDSize) + (!_GraphDescription->_FlipHorizontally ? 1.f : -1.f)) * LEDSize;
                        }

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }

                    Rect.x1 = FullRect.x1;
                }
            }

            Rect.y1 = Rect.y2 + _State->_GaugeGap;
        }
    }
    else
    {
        const FLOAT n = std::ceilf(GetHeight() / LEDSize);
        const FLOAT dy = !_GraphDescription->_FlipVertically ? GetHeight() - (n * LEDSize) : (n * LEDSize) - GetHeight();

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.x2 = std::clamp(Rect.x1 + gaugeMetrics._BarWidth, 0.f, GetWidth());
            Rect.y2 = GetHeight();

            const rect_t FullRect = Rect;

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    deviceContext->FillRectangle(FullRect, _BackgroundStyle->_Brush);
                else
                    deviceContext->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);
            }

            // Draw the peak.
            {
                // Draw the foreground (Peak).
                if (_PeakStyle->IsEnabled())
                {
                    const double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, gaugeMetrics._dBFSZero) : Value.PeakRender;

                    Rect.y2 = (FLOAT) (PeakRender * GetHeight()) + dy;

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _PeakStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                            Rect.y2 = std::ceilf((Rect.y2 / LEDSize) + (!_GraphDescription->_FlipVertically ? 1.f : -1.f)) * LEDSize;

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }
                }

                // Draw the foreground (Peak, 0dBFS)
                if ((Value.PeakRender > gaugeMetrics._dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.y1 = (FLOAT) (Value.PeakRender       * GetHeight()) + dy;
                    Rect.y2 = (FLOAT) (gaugeMetrics._dBFSZero * GetHeight()) + dy;

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                        {
                            Rect.y1 = std::ceilf((Rect.y1 / LEDSize) + (!_GraphDescription->_FlipVertically ? 1.f : -1.f)) * LEDSize;
                            Rect.y2 = std::ceilf((Rect.y2 / LEDSize) + (!_GraphDescription->_FlipVertically ? 1.f : -1.f)) * LEDSize;
                        }

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }

                    Rect.y1 = FullRect.y1;
                }

                // Draw the foreground (Peak Top).
                if ((_State->_PeakMode != PeakMode::None) && (Value.MaxPeakRender > 0.) && _MaxPeakStyle->IsEnabled())
                {
                    Rect.y1 =
                    Rect.y2 = (FLOAT) ( Value.MaxPeakRender * GetHeight());

                    Rect.y1 = std::ceilf(std::clamp(Rect.y1 - PeakThickness, 0.f, GetHeight()));
                    Rect.y2 = std::ceilf(std::clamp(Rect.y2 + PeakThickness, 0.f, GetHeight()));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) Value.Opacity : _MaxPeakStyle->_Opacity;

                    _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                    deviceContext->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                    Rect.y1 = FullRect.y1;
                }
            }

            // Draw the RMS.
            {
                // Draw the foreground (RMS).
                if (_RMSStyle->IsEnabled())
                {
                    const double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, gaugeMetrics._dBFSZero) : Value.RMSRender;

                    Rect.y2 = (FLOAT) (RMSRender * GetHeight()) + dy;

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _RMSStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                            Rect.y2 = (std::ceilf(Rect.y2 / LEDSize) + (!_GraphDescription->_FlipVertically ? 1.f : -1.f)) * LEDSize;

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }
                }

                // Draw the foreground (RMS, 0dBFS)
                if ((Value.RMSRender > gaugeMetrics._dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.y1 = (FLOAT) (gaugeMetrics._dBFSZero * GetHeight());
                    Rect.y2 = (FLOAT) (Value.RMSRender * GetHeight());

                    if (!_State->_LEDMode)
                        deviceContext->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                    {
                        if (_State->_LEDIntegralSize)
                        {
                            Rect.y1 = (std::ceilf(Rect.y1 / LEDSize) + (!_GraphDescription->_FlipVertically ? 1.f : -1.f)) * LEDSize;
                            Rect.y2 = (std::ceilf(Rect.y2 / LEDSize) + (!_GraphDescription->_FlipVertically ? 1.f : -1.f)) * LEDSize;
                        }

                        deviceContext->PushAxisAlignedClip(Rect, D2D1_ANTIALIAS_MODE_ALIASED);

                        deviceContext->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, FullRect, FullRect);

                        deviceContext->PopAxisAlignedClip();
                    }

                    Rect.y1 = FullRect.y1;
                }
            }

            Rect.x1 = Rect.x2 + _State->_GaugeGap;
        }
    }
}
/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT gauge_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(deviceContext);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeBackground, deviceContext, _Size, L"", 1.f, &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugePeakLevel, deviceContext, _Size, L"", 1.f, &_PeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Gauge0dBPeakLevel, deviceContext, _Size, L"", 1.f, &_Peak0dBStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeMaxPeakLevel, deviceContext, _Size, L"", 1.f, &_MaxPeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeRMSLevel, deviceContext, _Size, L"", 1.f, &_RMSStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Gauge0dBRMSLevel, deviceContext, _Size, L"", 1.f, &_RMS0dBStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Deletes the device specific resources.
/// </summary>
void gauge_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_RMSStyle)
    {
        _RMSStyle->DeleteDeviceSpecificResources();
        _RMSStyle = nullptr;
    }

    if (_MaxPeakStyle)
    {
        _MaxPeakStyle->DeleteDeviceSpecificResources();
        _MaxPeakStyle = nullptr;
    }

    if (_Peak0dBStyle)
    {
        _Peak0dBStyle->DeleteDeviceSpecificResources();
        _Peak0dBStyle = nullptr;
    }

    if (_PeakStyle)
    {
        _PeakStyle->DeleteDeviceSpecificResources();
        _PeakStyle = nullptr;
    }

    if (_BackgroundStyle)
    {
        _BackgroundStyle->DeleteDeviceSpecificResources();
        _BackgroundStyle = nullptr;
    }

    _OpacityMask.Release();
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT gauge_t::CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = deviceContext->CreateCompatibleRenderTarget(D2D1::SizeF(_Size.width, _Size.height), &rt);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->Clear();

            const FLOAT LEDSize = _State->_LEDSize + _State->_LEDGap;

            if (LEDSize > 0.f)
            {
                if (_State->_HorizontalPeakMeter)
                {
                    for (FLOAT x = _State->_LEDGap; x < _Size.width; x += LEDSize)
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDSize, _Size.height), Brush);
                }
                else
                {
                    for (FLOAT y = _State->_LEDGap; y < _Size.height; y += LEDSize)
                        rt->FillRectangle(D2D1::RectF(0.f, y, _Size.width, y + _State->_LEDSize), Brush);
                }
            }

            hr = rt->EndDraw();
        }

        if (SUCCEEDED(hr))
            hr = rt->GetBitmap(&_OpacityMask);
    }

    return hr;
}

/// <summary>
/// Gets the metrics used to render the gauges.
/// </summary>
bool gauge_t::GetMetrics(gauge_metrics_t & gm) const noexcept
{
    const FLOAT n = (FLOAT) _Analysis->_GaugeValues.size();

    if (n == 0)
        return false;

    gm._dBFSZero = msc::Map(0., _GraphDescription->_AmplitudeLo, _GraphDescription->_AmplitudeHi, 0., 1.);

    gm._TotalBarGap = _State->_GaugeGap * (FLOAT) (n - 1);
    gm._TickSize = 4.f;
    gm._TotalTickSize = (FLOAT) (_GraphDescription->_YAxisLeft + _GraphDescription->_YAxisRight) * gm._TickSize;

    gm._BarHeight = (GetHeight() - gm._TotalBarGap - gm._TotalTickSize) / n;
    gm._BarWidth  = (GetWidth()  - gm._TotalBarGap - gm._TotalTickSize) / n;

    gm._TotalBarHeight = (gm._BarHeight * n) + gm._TotalBarGap;
    gm._TotalBarWidth  = (gm._BarWidth  * n) + gm._TotalBarGap;

    gm._Offset = (_State->_HorizontalPeakMeter ? (GetHeight() - gm._TotalBarHeight) : (GetWidth() - gm._TotalBarWidth)) / 2.f;

    return true;
}

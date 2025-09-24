
/** $VER: Gauges.cpp (2025.09.24) P. Stuer - Implements the gauges of the peak meter. **/

#include "pch.h"

#include "Gauges.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void gauge_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void gauge_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetBounds(rect);
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

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void gauge_t::Render(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    if (_Analysis->_GaugeValues.empty() || (GetWidth() <= 0.f) || (GetHeight() <= 0.f))
        return;

    const FLOAT PeakThickness = _MaxPeakStyle->_Thickness / 2.f;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

#ifdef _DEBUG
    renderTarget->FillRectangle({ 0.f, 0.f, 16.f, 16.f }, _DebugBrush); // Top/Left indicator
#endif

    if (_State->_HorizontalPeakMeter)
    {
        rect_t Rect = {  };

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.y2 = std::clamp(Rect.y1 + gaugeMetrics._BarHeight, 0.f, GetHeight());

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                Rect.x2 = GetWidth();

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            // Draw the peak.
            {
                double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, gaugeMetrics._dBFSZero) : Value.PeakRender;

                // Draw the foreground (Peak).
                if (_PeakStyle->IsEnabled())
                {
                    Rect.x2 = (FLOAT) (PeakRender * GetWidth());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the foreground (Peak, 0dBFS)
                if ((Value.PeakRender > gaugeMetrics._dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.x2 = (FLOAT) (Value.PeakRender * GetWidth());

                    FLOAT OldLeft = Rect.x1;

                    Rect.x1 = (FLOAT) (gaugeMetrics._dBFSZero * GetWidth());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.x1 = OldLeft;
                }

                // Draw the foreground (Peak Top).
                if ((_State->_PeakMode != PeakMode::None) && (Value.MaxPeakRender > 0.) && _MaxPeakStyle->IsEnabled())
                {
                    FLOAT OldLeft = Rect.x1;

                    Rect.x1 =
                    Rect.x2 = (FLOAT) (Value.MaxPeakRender * GetWidth());

                    Rect.x1 = ::ceil(std::clamp(Rect.x1 - PeakThickness, 0.f, GetWidth()));
                    Rect.x2 = ::ceil(std::clamp(Rect.x2 + PeakThickness, 0.f, GetWidth()));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) Value.Opacity : _MaxPeakStyle->_Opacity;

                    _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                    renderTarget->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                    Rect.x1 = OldLeft;
                }
            }

            // Draw the RMS.
            {
                double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, gaugeMetrics._dBFSZero) : Value.RMSRender;

                // Draw the foreground (RMS).
                if (_RMSStyle->IsEnabled())
                {
                    Rect.x2 = (FLOAT) (RMSRender * GetWidth());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the foreground (RMS, 0dBFS)
                if ((Value.RMSRender > gaugeMetrics._dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.x2 = (FLOAT) (Value.RMSRender * GetWidth());

                    FLOAT OldLeft = Rect.x1;

                    Rect.x1 = (FLOAT) (gaugeMetrics._dBFSZero * GetWidth());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.x1 = OldLeft;
                }
            }

            Rect.y1 = Rect.y2 + _State->_GaugeGap;
        }
    }
    else
    {
        rect_t Rect = {  };

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.x2 = std::clamp(Rect.x1 + gaugeMetrics._BarWidth, 0.f, GetWidth());

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                Rect.y2 = GetHeight();

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            // Draw the peak.
            {
                double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, gaugeMetrics._dBFSZero) : Value.PeakRender;

                // Draw the foreground (Peak).
                if (_PeakStyle->IsEnabled())
                {
                    Rect.y2 = (FLOAT) (PeakRender * GetHeight());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the foreground (Peak, 0dBFS)
                if ((Value.PeakRender > gaugeMetrics._dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.y2 = (FLOAT) (Value.PeakRender * GetHeight());

                    FLOAT OldTop = Rect.y1;

                    Rect.y1 = (FLOAT) (gaugeMetrics._dBFSZero * GetHeight());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.y1 = OldTop;
                }

                // Draw the foreground (Peak Top).
                if ((_State->_PeakMode != PeakMode::None) && (Value.MaxPeakRender > 0.) && _MaxPeakStyle->IsEnabled())
                {
                    FLOAT OldTop = Rect.y1;

                    Rect.y1 =
                    Rect.y2 = (FLOAT) ( Value.MaxPeakRender * GetHeight());

                    Rect.y1 = ::ceil(std::clamp(Rect.y1 - PeakThickness, 0.f, GetHeight()));
                    Rect.y2 = ::ceil(std::clamp(Rect.y2 + PeakThickness, 0.f, GetHeight()));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) Value.Opacity : _MaxPeakStyle->_Opacity;

                    _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                    renderTarget->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                    Rect.y1 = OldTop;
                }
            }

            // Draw the RMS.
            {
                double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, gaugeMetrics._dBFSZero) : Value.RMSRender;

                // Draw the foreground (RMS).
                if (_RMSStyle->IsEnabled())
                {
                    Rect.y2 = (FLOAT) (RMSRender * GetHeight());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the foreground (RMS, 0dBFS)
                if ((Value.RMSRender > gaugeMetrics._dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.y2 = (FLOAT) (Value.RMSRender * GetHeight());

                    FLOAT OldTop = Rect.y1;

                    Rect.y1 = (FLOAT) (gaugeMetrics._dBFSZero * GetHeight());

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.y1 = OldTop;
                }
            }

            Rect.x1 = Rect.x2 + _State->_GaugeGap;
        }
    }
}
/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT gauge_t::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(renderTarget);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeBackground, renderTarget, _Size, L"", &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugePeakLevel, renderTarget, _Size, L"", &_PeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Gauge0dBPeakLevel, renderTarget, _Size, L"", &_Peak0dBStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeMaxPeakLevel, renderTarget, _Size, L"", &_MaxPeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeRMSLevel, renderTarget, _Size, L"", &_RMSStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Gauge0dBRMSLevel, renderTarget, _Size, L"", &_RMS0dBStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void gauge_t::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_RMSStyle)
    {
        _RMSStyle->ReleaseDeviceSpecificResources();
        _RMSStyle = nullptr;
    }

    if (_MaxPeakStyle)
    {
        _MaxPeakStyle->ReleaseDeviceSpecificResources();
        _MaxPeakStyle = nullptr;
    }

    if (_Peak0dBStyle)
    {
        _Peak0dBStyle->ReleaseDeviceSpecificResources();
        _Peak0dBStyle = nullptr;
    }

    if (_PeakStyle)
    {
        _PeakStyle->ReleaseDeviceSpecificResources();
        _PeakStyle = nullptr;
    }

    if (_BackgroundStyle)
    {
        _BackgroundStyle->ReleaseDeviceSpecificResources();
        _BackgroundStyle = nullptr;
    }

    _OpacityMask.Release();
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT gauge_t::CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept
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
                if (_State->_HorizontalPeakMeter)
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

/// <summary>
/// Gets the metrics used to render the gauges.
/// </summary>
bool gauge_t::GetMetrics(gauge_metrics_t & gm) const noexcept
{
    const FLOAT n = (FLOAT) _Analysis->_GaugeValues.size();

    if (n == 0)
        return false;

    gm._dBFSZero = msc::Map(0., _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0., 1.);

    gm._TotalBarGap = _State->_GaugeGap * (FLOAT) (n - 1);
    gm._TickSize = 4.f;
    gm._TotalTickSize = (FLOAT) (_GraphSettings->_YAxisLeft + _GraphSettings->_YAxisRight) * gm._TickSize;

    gm._BarHeight = (GetHeight() - gm._TotalBarGap - gm._TotalTickSize) / n;
    gm._BarWidth  = (GetWidth()  - gm._TotalBarGap - gm._TotalTickSize) / n;

    gm._TotalBarHeight = (gm._BarHeight * n) + gm._TotalBarGap;
    gm._TotalBarWidth  = (gm._BarWidth  * n) + gm._TotalBarGap;

    gm._Offset = (_State->_HorizontalPeakMeter ? (GetHeight() - gm._TotalBarHeight) : (GetWidth() - gm._TotalBarWidth)) / 2.f;

    return true;
}

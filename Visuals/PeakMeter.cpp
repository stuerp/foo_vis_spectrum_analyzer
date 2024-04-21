
/** $VER: PeakMeter.cpp (2024.04.21) P. Stuer - Represents a peak meter. **/

#include "framework.h"
#include "PeakMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

#pragma region Peak Meter

PeakMeter::PeakMeter()
{
    _Bounds = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void PeakMeter::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
    _Gauges.Initialize(state, settings, analysis);
    _GaugeScales.Initialize(state, settings, analysis);
    _GaugeNames.Initialize(state, settings, analysis);
    _RMSReadOut.Initialize(state, settings, analysis);
    _PeakReadOut.Initialize(state, settings, analysis);

    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void PeakMeter::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);

    _Gauges.SetBounds(rect);
    _GaugeNames.SetBounds(rect);
    _GaugeScales.SetBounds(rect);
    _RMSReadOut.SetBounds(rect);
    _PeakReadOut.SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakMeter::Reset()
{
    _IsResized = true;

    _Gauges.Reset();
    _GaugeNames.Reset();
    _GaugeScales.Reset();
    _RMSReadOut.Reset();
    _PeakReadOut.Reset();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void PeakMeter::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    D2D1_RECT_F Rect = _Bounds;

    // Gauge Name metrics
    {
        if (_State->_HorizontalPeakMeter)
            Rect.bottom -= (FLOAT) (_GraphSettings->_YAxisLeft + _GraphSettings->_YAxisRight) * _GaugeScales.GetTextHeight();
        else
            Rect.right  -= (FLOAT) (_GraphSettings->_YAxisLeft + _GraphSettings->_YAxisRight) * _GaugeScales.GetTextWidth();

        _GaugeNames.SetBounds(Rect);
        _GaugeNames.Resize();
    }

    // RMS Read Out metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            Rect.left  = 0.f;
            Rect.right = _RMSReadOut.GetTextWidth();
        }

        _RMSReadOut.SetBounds(Rect);
        _RMSReadOut.Resize();
    }

    // Peak Read Out metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            Rect.left  = 0.f;
            Rect.right = _PeakReadOut.GetTextWidth();
        }

        _PeakReadOut.SetBounds(Rect);
        _PeakReadOut.Resize();
    }

    // Gauge scale metrics
    {
        Rect = _Bounds;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                Rect.right -= _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                Rect.right -= _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                Rect.right -= 4.f;

            if (_GraphSettings->_XAxisTop)
                Rect.right -= _GaugeNames.GetTextWidth();
        }
        else
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                Rect.bottom -= _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                Rect.bottom -= _PeakReadOut.GetTextHeight();

            if (_GraphSettings->_XAxisTop)
                Rect.bottom -= _GaugeNames.GetTextHeight();
        }

        _GaugeScales.SetBounds(Rect);
        _GaugeScales.Resize();
    }

    // Gauge metrics
    {
        Rect = _Bounds;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                Rect.right -= _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                Rect.right -= _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                Rect.right -= 4.f;

            if (_GraphSettings->_XAxisTop)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_GraphSettings->_YAxisRight)
                Rect.bottom -= _GaugeScales.GetTextHeight();

            if (_GraphSettings->_YAxisLeft)
                Rect.bottom -= _GaugeScales.GetTextHeight();
        }
        else
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                Rect.bottom -= _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                Rect.bottom -= _PeakReadOut.GetTextHeight();

            if (_GraphSettings->_XAxisTop)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_GraphSettings->_YAxisRight)
                Rect.right -= _GaugeScales.GetTextWidth();

            if (_GraphSettings->_YAxisLeft)
                Rect.right -= _GaugeScales.GetTextWidth();
        }

        _Gauges.SetBounds(Rect);
        _Gauges.Resize();
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void PeakMeter::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    _Gauges.GetGaugeMetrics(_GaugeMetrics);

    // Render the gauge scales.
    {
        // Translate from client coordinates to element coordinates.
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_FlipHorizontally)
            {
                if (_GraphSettings->_XAxisTop)
                    dx += _GaugeNames.GetTextWidth();
            }
            else
            {
                if (_GraphSettings->_XAxisBottom)
                    dx += _GaugeNames.GetTextWidth();
    
                if (_RMSReadOut.IsVisible())
                    dx += _RMSReadOut.GetTextWidth();
    
                if (_PeakReadOut.IsVisible())
                    dx += _PeakReadOut.GetTextWidth();

                if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                    dx += 4.f;
            }
        }
        else
        {
            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_XAxisBottom)
                    dy += _GaugeNames.GetTextHeight();

                if (_RMSReadOut.IsVisible())
                    dy += _RMSReadOut.GetTextHeight();
    
                if (_PeakReadOut.IsVisible())
                    dy += _PeakReadOut.GetTextHeight();
            }
            else
            {
                if (_GraphSettings->_XAxisTop)
                    dy += _GaugeNames.GetTextHeight();
            }
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(dx, dy);

        renderTarget->SetTransform(Translate);

        _GaugeScales.Render(renderTarget);
    }

    // Render the gauges.
    {
        // Translate from client coordinates to element coordinates.
        D2D1::Matrix3x2F Transform = D2D1::Matrix3x2F::Identity();

        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_FlipHorizontally)
                Transform =  D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, GetWidth(), 0.f);

            if (_GraphSettings->_FlipVertically)
                Transform = Transform * D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, GetHeight());

            if (_GraphSettings->_XAxisBottom)
                dx += (_GraphSettings->_FlipHorizontally) ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                dx += (_GraphSettings->_FlipHorizontally) ? -_RMSReadOut.GetTextWidth() : _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                dx += (_GraphSettings->_FlipHorizontally) ? -_PeakReadOut.GetTextWidth() : _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                dx += 4.f;

            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? -(_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphSettings->_FlipHorizontally)
                Transform =  D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, GetWidth(), 0.f);

            if (!_GraphSettings->_FlipVertically)
                Transform = Transform * D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, GetHeight());

            if (_GraphSettings->_XAxisBottom)
                dy -= (_GraphSettings->_FlipVertically) ? -_GaugeNames.GetTextHeight() : _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                dy -= (_GraphSettings->_FlipVertically) ? -_RMSReadOut.GetTextHeight() : _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                dy -= (_GraphSettings->_FlipVertically) ? -_PeakReadOut.GetTextHeight() : _PeakReadOut.GetTextHeight();

            if (_GraphSettings->_YAxisLeft)
                dx += (_GraphSettings->_FlipHorizontally) ? -(_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(dx, dy);

        renderTarget->SetTransform(Transform * Translate);

        _Gauges.Render(renderTarget, _GaugeMetrics);
    }

    // Render the gauge names.
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                dx += (_GraphSettings->_FlipHorizontally) ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(dx, dy);

        renderTarget->SetTransform(Translate);

        _GaugeNames.Render(renderTarget, _GaugeMetrics);
    }

    // Render the RMS read out.
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_FlipHorizontally)
                dx = GetWidth() - _RMSReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom)
                dx += _GraphSettings->_FlipHorizontally ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                dx += _GraphSettings->_FlipHorizontally ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);

            if (_GraphSettings->_XAxisBottom)
                dy += _GraphSettings->_FlipVertically ? _GaugeNames.GetTextHeight() : -_GaugeNames.GetTextHeight();
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(dx, dy);

        renderTarget->SetTransform(Translate);

        _RMSReadOut.Render(renderTarget, _GaugeMetrics);
    }

    // Render the Peak read out.
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_FlipHorizontally)
                dx = GetWidth() - _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom)
                dx += _GraphSettings->_FlipHorizontally ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                dx += _GraphSettings->_FlipHorizontally ? -_RMSReadOut.GetTextWidth() : _RMSReadOut.GetTextWidth();

            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                dx += _GraphSettings->_FlipHorizontally ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);

            if (_GraphSettings->_XAxisBottom)
                dy += _GraphSettings->_FlipVertically ? _GaugeNames.GetTextHeight() : -_GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                dy += _GraphSettings->_FlipVertically ? _RMSReadOut.GetTextHeight() : -_RMSReadOut.GetTextHeight();
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(dx, dy);

        renderTarget->SetTransform(Translate);

        _PeakReadOut.Render(renderTarget, _GaugeMetrics);
    }

    ResetTransform(renderTarget);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT PeakMeter::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = _GaugeNames.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _RMSReadOut.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _PeakReadOut.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _GaugeScales.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _Gauges.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void PeakMeter::ReleaseDeviceSpecificResources() noexcept
{
    _Gauges.ReleaseDeviceSpecificResources();

    _GaugeScales.ReleaseDeviceSpecificResources();

    _PeakReadOut.ReleaseDeviceSpecificResources();

    _RMSReadOut.ReleaseDeviceSpecificResources();

    _GaugeNames.ReleaseDeviceSpecificResources();
}

#pragma endregion

#pragma region Gauges

/// <summary>
/// Initializes this instance.
/// </summary>
void Gauges::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void Gauges::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void Gauges::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void Gauges::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void Gauges::Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    if ((_Analysis->_GaugeValues.size() == 0) || (GetWidth() <= 0.f) || (GetHeight() <= 0.f))
        return;

    const FLOAT PeakThickness = _MaxPeakStyle->_Thickness / 2.f;

    auto OldAntialiasMode = renderTarget->GetAntialiasMode();

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

#ifdef _DEBUG
    renderTarget->FillRectangle({ 0.f, 0.f, 16.f, 16.f }, _DebugBrush); // Top/Left indicator
#endif

    if (_State->_HorizontalPeakMeter)
    {
        BOUNDS Rect = {  };

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
        BOUNDS Rect = {  };

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

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(OldAntialiasMode);
}
/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT Gauges::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
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
void Gauges::ReleaseDeviceSpecificResources() noexcept
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
HRESULT Gauges::CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept
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
void Gauges::GetGaugeMetrics(GaugeMetrics & gm) const noexcept
{
    gm._dBFSZero = Map(0., _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0., 1.);

    const FLOAT n = (FLOAT) _Analysis->_GaugeValues.size();

    gm._TotalBarGap = _State->_GaugeGap * (FLOAT) (n - 1);
    gm._TickSize = 4.f;
    gm._TotalTickSize = (FLOAT) (_GraphSettings->_YAxisLeft + _GraphSettings->_YAxisRight) * gm._TickSize;

    gm._BarHeight = ::floor((GetHeight() - gm._TotalBarGap - gm._TotalTickSize) / n);
    gm._BarWidth  = ::floor((GetWidth()  - gm._TotalBarGap - gm._TotalTickSize) / n);

    gm._TotalBarHeight = (gm._BarHeight * n) + gm._TotalBarGap;
    gm._TotalBarWidth  = (gm._BarWidth  * n) + gm._TotalBarGap;

    gm._Offset = _State->_HorizontalPeakMeter ? ::floor((GetHeight() - gm._TotalBarHeight) / 2.f): ::floor((GetWidth() - gm._TotalBarWidth) / 2.f);
}

#pragma endregion

#pragma region Gauge Scales

/// <summary>
/// Initializes this instance.
/// </summary>
void GaugeScales::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();

    _Labels.clear();

    if (_GraphSettings->_YAxisMode == YAxisMode::None)
        return;

    // Create the labels.
    {
        for (double Amplitude = _GraphSettings->_AmplitudeLo; Amplitude <= _GraphSettings->_AmplitudeHi; Amplitude -= _GraphSettings->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%+d", (int) Amplitude);

            Label lb = { Text, Amplitude };

            _Labels.push_back(lb);
        }
    }
}

/// <summary>
/// Moves this instance.
/// </summary>
void GaugeScales::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void GaugeScales::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void GaugeScales::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    if (_State->_HorizontalPeakMeter)
    {
        const FLOAT cx = (_TextStyle->GetWidth() / 2.f);

        // Calculate the position of the labels based on the width.
        D2D1_RECT_F OldRect = {  };

        const FLOAT xMin = !_GraphSettings->_FlipHorizontally ? 0.f : GetWidth();
        const FLOAT xMax = !_GraphSettings->_FlipHorizontally ? GetWidth() : 0.f;

        for (Label & Iter : _Labels)
        {
            FLOAT x = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., xMin, xMax);

            // Don't generate any labels outside the bounds.
            if (!InRange(x, 0.f, GetWidth()))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.Point1 = D2D1_POINT_2F(x, _TextStyle->GetHeight());
            Iter.Point2 = D2D1_POINT_2F(x, GetHeight() - _TextStyle->GetHeight());

            x -= cx;

            Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;

            if (!_GraphSettings->_XAxisBottom)
            {
                if (!_GraphSettings->_FlipHorizontally && (x <= 0.f))
                {
                    x = 0.f;
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
                }
                else
                if (_GraphSettings->_FlipHorizontally && ((x + _TextStyle->GetWidth()) > GetWidth()))
                {
                    x = GetWidth() - _TextStyle->GetWidth();
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
                }
            }

            if (!_GraphSettings->_XAxisTop)
            {
                if (_GraphSettings->_FlipHorizontally && (x <= 0.f))
                {
                    x = 0.f;
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
                }
                else
                if (!_GraphSettings->_FlipHorizontally && ((x + _TextStyle->GetWidth()) > GetWidth()))
                {
                    x = GetWidth() - _TextStyle->GetWidth();
                    Iter._HAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
                }
            }

            Iter.Rect1 = { x, 0.f,                                   x + _TextStyle->GetWidth(), _TextStyle->GetHeight() };
            Iter.Rect2 = { x, GetHeight() - _TextStyle->GetHeight(), x + _TextStyle->GetWidth(), GetHeight() };

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingHorizontally(Iter.Rect1, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.Rect1;
        }
    }
    else
    {
        const FLOAT cy = (_TextStyle->GetHeight() / 2.f);

        // Calculate the position of the labels based on the height.
        D2D1_RECT_F OldRect = {  };

        const FLOAT yMin = !_GraphSettings->_FlipVertically ? GetHeight() : 0.f;
        const FLOAT yMax = !_GraphSettings->_FlipVertically ? 0.f : GetHeight();

        for (Label & Iter : _Labels)
        {
            FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., yMin, yMax);

            // Don't generate any labels outside the bounds.
            if (!InRange(y, 0.f, GetHeight()))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.Point1 = D2D1_POINT_2F(_TextStyle->GetWidth(),              y);
            Iter.Point2 = D2D1_POINT_2F(GetWidth() - _TextStyle->GetWidth(), y);

            y -= cy;

            Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;

            if (!_GraphSettings->_XAxisBottom)
            {
                if (_GraphSettings->_FlipVertically && (y <= 0.f))
                {
                    y = 0.f;
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                }
                else
                if (!_GraphSettings->_FlipVertically && ((y + _TextStyle->GetHeight()) > GetHeight()))
                {
                    y = GetHeight() - _TextStyle->GetHeight();
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
                }
            }

            if (!_GraphSettings->_XAxisTop)
            {
                if (!_GraphSettings->_FlipVertically && (y <= 0.f))
                {
                    y = 0.f;
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                }
                else
                if (_GraphSettings->_FlipVertically && ((y + _TextStyle->GetHeight()) > GetHeight()))
                {
                    y = GetHeight() - _TextStyle->GetHeight();
                    Iter._VAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
                }
            }

            Iter.Rect1 = { 0.f,                                 y, _TextStyle->GetWidth(), y + _TextStyle->GetHeight() };
            Iter.Rect2 = { GetWidth() - _TextStyle->GetWidth(), y, GetWidth(),             y + _TextStyle->GetHeight() };

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingVertically(Iter.Rect1, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.Rect1;
        }
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void GaugeScales::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    if ((_GraphSettings->_YAxisMode == YAxisMode::None) || (!_GraphSettings->_YAxisLeft && !_GraphSettings->_YAxisRight))
        return;

    if (_State->_HorizontalPeakMeter)
    {
        for (const Label & Iter : _Labels)
        {
            // Draw the vertical grid line.
            if (_LineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.Point1, Iter.Point2, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                _TextStyle->SetHorizontalAlignment(Iter._HAlignment);

                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect1, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    
                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect2, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }
    else
    {
        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            if (_LineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.Point1, Iter.Point2, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                _TextStyle->SetVerticalAlignment(Iter._VAlignment);

                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect1, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect2, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }
}

HRESULT GaugeScales::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, Size, L"+999", &_TextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, Size, L"", &_LineStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void GaugeScales::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_LineStyle)
    {
        _LineStyle->ReleaseDeviceSpecificResources();
        _LineStyle = nullptr;
    }

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}

#pragma endregion

#pragma region Gauge Names

/// <summary>
/// Initializes this instance.
/// </summary>
void GaugeNames::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void GaugeNames::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void GaugeNames::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void GaugeNames::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void GaugeNames::Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    if ((_GraphSettings->_XAxisMode == XAxisMode::None) || (!_GraphSettings->_XAxisTop && !_GraphSettings->_XAxisBottom))
        return;

    if (_State->_HorizontalPeakMeter)
        RenderHorizontal(renderTarget, gaugeMetrics);
    else
        RenderVertical(renderTarget, gaugeMetrics);
}

/// <summary>
/// Draws the channel names. Note: Rendered in absolute coordinates! No transformation.
/// </summary>
void GaugeNames::RenderHorizontal(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.top = _GraphSettings->_FlipVertically ? _Size.height : 0.f;

    const FLOAT dy = _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight : gaugeMetrics._BarHeight;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.bottom = std::clamp(Rect.top + dy, 0.f, _Size.height);

        if (_TextStyle->IsEnabled())
        {
            if (_GraphSettings->_XAxisTop)
            {
                Rect.left  = _GraphSettings->_FlipHorizontally ? 0.f : GetWidth() - _TextStyle->GetWidth();
                Rect.right = _GraphSettings->_FlipHorizontally ? _TextStyle->GetWidth() : GetWidth();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_GraphSettings->_XAxisBottom)
            {
                Rect.left  = _GraphSettings->_FlipHorizontally ? GetWidth() - _TextStyle->GetWidth() : 0.f;
                Rect.right = _GraphSettings->_FlipHorizontally ? GetWidth()                          : _TextStyle->GetWidth();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        Rect.top += _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight - _State->_GaugeGap : gaugeMetrics._BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Draws the channel names.
/// </summary>
void GaugeNames::RenderVertical(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.left = _GraphSettings->_FlipHorizontally ? _Size.width: 0.f;

    const FLOAT dx = _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth : gaugeMetrics._BarWidth;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.right = std::clamp(Rect.left + dx, 0.f, _Size.width);

        if (_TextStyle->IsEnabled())
        {
            if (_GraphSettings->_XAxisTop)
            {
                Rect.top    = _GraphSettings->_FlipVertically ? GetHeight() - _TextStyle->GetHeight() : 0.f;
                Rect.bottom = _GraphSettings->_FlipVertically ? GetHeight()                           : _TextStyle->GetHeight();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_GraphSettings->_XAxisBottom)
            {
                Rect.top    = _GraphSettings->_FlipVertically ? 0.f                     : GetHeight() - _TextStyle->GetHeight();
                Rect.bottom = _GraphSettings->_FlipVertically ? _TextStyle->GetHeight() : GetHeight();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        Rect.left += _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth - _State->_GaugeGap : gaugeMetrics._BarWidth + _State->_GaugeGap;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT GaugeNames::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, Size, L"LFE", &_TextStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void GaugeNames::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}

#pragma endregion

#pragma region RMS Read Out

/// <summary>
/// Initializes this instance.
/// </summary>
void RMSReadOut::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void RMSReadOut::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void RMSReadOut::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void RMSReadOut::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void RMSReadOut::Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr) || !IsVisible())
        return;

    if (_State->_HorizontalPeakMeter)
        RenderHorizontal(renderTarget, gaugeMetrics);
    else
        RenderVertical(renderTarget, gaugeMetrics);
}

/// <summary>
/// Render the RMS read out.
/// </summary>
void RMSReadOut::RenderHorizontal(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.top = _GraphSettings->_FlipVertically ? GetHeight() : 0.f;

    const FLOAT dy = _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight : gaugeMetrics._BarHeight;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _TextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.bottom = std::clamp(Rect.top + dy, 0.f, GetHeight());

        // Draw the RMS text display.
        if (_TextStyle->IsEnabled())
        {
            Rect.left  = _GraphSettings->_FlipHorizontally ? GetWidth() - _TextStyle->GetWidth() : 0.f;
            Rect.right = _GraphSettings->_FlipHorizontally ? GetWidth()                          : _TextStyle->GetWidth();

            WCHAR Text[16];

            if (::isfinite(gv.RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.RMS);
            else
                ::wcscpy_s(Text, _countof(Text), L"-∞");

//          renderTarget->FillRectangle(Rect, _DebugBrush);
            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.top += _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight - _State->_GaugeGap : gaugeMetrics._BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Render the RMS read out.
/// </summary>
void RMSReadOut::RenderVertical(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.left = _GraphSettings->_FlipHorizontally ? GetWidth() : 0.f;

    const FLOAT dx = _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth : gaugeMetrics._BarWidth;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _TextStyle->SetVerticalAlignment(_GraphSettings->_FlipVertically ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.right = std::clamp(Rect.left + dx, 0.f, GetWidth());

        // Draw the RMS text display.
        if (_TextStyle->IsEnabled())
        {
            Rect.top    = _GraphSettings->_FlipVertically ? 0.f                     : GetHeight() - _TextStyle->GetHeight();
            Rect.bottom = _GraphSettings->_FlipVertically ? _TextStyle->GetHeight() : GetHeight();

            WCHAR Text[16];

            if (::isfinite(gv.RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.RMS);
            else
                ::wcscpy_s(Text, _countof(Text), L"-∞");

//          renderTarget->FillRectangle(Rect, _DebugBrush);
            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.left += _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth - _State->_GaugeGap : gaugeMetrics._BarWidth + _State->_GaugeGap;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT RMSReadOut::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeRMSLevelText, renderTarget, Size, L"+199.9", &_TextStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void RMSReadOut::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}

#pragma endregion

#pragma region Peak Read Out

/// <summary>
/// Initializes this instance.
/// </summary>
void PeakReadOut::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void PeakReadOut::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakReadOut::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void PeakReadOut::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void PeakReadOut::Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr) || !IsVisible())
        return;

    if (_State->_HorizontalPeakMeter)
        RenderHorizontal(renderTarget, gaugeMetrics);
    else
        RenderVertical(renderTarget, gaugeMetrics);
}

/// <summary>
/// Render the RMS read out.
/// </summary>
void PeakReadOut::RenderHorizontal(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.top = _GraphSettings->_FlipVertically ? _Size.height : 0.f;

    const FLOAT dy = _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight : gaugeMetrics._BarHeight;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _TextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.bottom = std::clamp(Rect.top + dy, 0.f, _Size.height);

        // Draw the RMS text display.
        if (_TextStyle->IsEnabled())
        {
            Rect.left  = _GraphSettings->_FlipHorizontally ? GetWidth() - _TextStyle->GetWidth() : 0.f;
            Rect.right = _GraphSettings->_FlipHorizontally ? GetWidth()                          : _TextStyle->GetWidth();

            WCHAR Text[16];

            if (::isfinite(gv.RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.RMS);
            else
                ::wcscpy_s(Text, _countof(Text), L"-∞");

 //         renderTarget->FillRectangle(Rect, _DebugBrush);
            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.top += _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight - _State->_GaugeGap : gaugeMetrics._BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Render the RMS read out.
/// </summary>
void PeakReadOut::RenderVertical(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.left = _GraphSettings->_FlipHorizontally ? _Size.width: 0.f;

    const FLOAT dx = _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth : gaugeMetrics._BarWidth;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _TextStyle->SetVerticalAlignment(_GraphSettings->_FlipVertically ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.right = std::clamp(Rect.left + dx, 0.f, _Size.width);

        if (_TextStyle->IsEnabled())
        {

            Rect.top    = _GraphSettings->_FlipVertically ? 0.f                     : GetHeight() - _TextStyle->GetHeight();
            Rect.bottom = _GraphSettings->_FlipVertically ? _TextStyle->GetHeight() : GetHeight();

            WCHAR Text[16];

            if (::isfinite(gv.Peak))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.Peak);
            else
                ::wcscpy_s(Text, _countof(Text), L"-∞");

 //         renderTarget->FillRectangle(Rect, _DebugBrush);
            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.left += _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth - _State->_GaugeGap : gaugeMetrics._BarWidth + _State->_GaugeGap;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT PeakReadOut::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugePeakLevelText, renderTarget, Size, L"+199.9", &_TextStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void PeakReadOut::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}

#pragma endregion

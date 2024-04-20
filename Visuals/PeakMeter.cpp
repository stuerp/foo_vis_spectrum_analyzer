
/** $VER: PeakMeter.cpp (2024.04.20) P. Stuer - Represents a peak meter. **/

#include "framework.h"
#include "PeakMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

#pragma region Gauges

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
    _GaugeScales.Initialize(state, settings, analysis);
    _GaugeNames.Initialize(state, settings, analysis);

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
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakMeter::Reset()
{
    _IsResized = true;

    _GaugeNames.Reset();
    _GaugeScales.Reset();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void PeakMeter::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    // Gauge Name metrics
    {
        D2D1_RECT_F Rect = _Bounds;

        if (_State->_HorizontalPeakMeter)
            Rect.bottom -= (FLOAT) (_GraphSettings->_YAxisLeft + _GraphSettings->_YAxisRight) * _GaugeScales.GetTextHeight();
        else
            Rect.right  -= (FLOAT) (_GraphSettings->_YAxisLeft + _GraphSettings->_YAxisRight) * _GaugeScales.GetTextWidth();

        _GaugeNames.SetBounds(Rect);
        _GaugeNames.Resize();
    }

    // Gauge Scale metrics
    {
        D2D1_RECT_F Rect = _Bounds;

        if (_State->_HorizontalPeakMeter)
            Rect.right  -= 4 + (FLOAT) (_GraphSettings->_XAxisTop + _GraphSettings->_XAxisBottom) * _GaugeNames.GetTextWidth();
        else
            Rect.bottom -= (FLOAT) (_GraphSettings->_XAxisTop + _GraphSettings->_XAxisBottom) * _GaugeNames.GetTextHeight();

        _GaugeScales.SetBounds(Rect);
        _GaugeScales.Resize();
    }

    // Gauge metrics
    {
        _GBounds = _Bounds;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_XAxisBottom)
                if (_GraphSettings->_FlipHorizontally)
                    _GBounds.right -= _GaugeNames.GetTextWidth();
                else
                    _GBounds.left  += _GaugeNames.GetTextWidth();

            if (_GraphSettings->_XAxisTop)
                if (_GraphSettings->_FlipHorizontally)
                    _GBounds.left  += _GaugeNames.GetTextWidth();
                else
                    _GBounds.right -= _GaugeNames.GetTextWidth();

            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_YAxisRight)
                    _GBounds.top    += _GaugeScales.GetTextHeight();

                if (_GraphSettings->_YAxisLeft)
                    _GBounds.bottom -= _GaugeScales.GetTextHeight();
            }
            else
            {
                if (_GraphSettings->_YAxisLeft)
                    _GBounds.top    += _GaugeScales.GetTextHeight();

                if (_GraphSettings->_YAxisRight)
                    _GBounds.bottom -= _GaugeScales.GetTextHeight();
            }
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                _GBounds.left  += _GaugeScales.GetTextWidth();

            if (_GraphSettings->_YAxisRight)
                _GBounds.right -= _GaugeScales.GetTextWidth();

            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_XAxisBottom)
                    _GBounds.top += _GaugeNames.GetTextHeight();

                if (_GraphSettings->_XAxisTop)
                    _GBounds.bottom -= _GaugeNames.GetTextHeight();
            }
            else
            {
                if (_GraphSettings->_XAxisTop)
                    _GBounds.top += _GaugeNames.GetTextHeight();

                if (_GraphSettings->_XAxisBottom)
                    _GBounds.bottom -= _GaugeNames.GetTextHeight();
            }
        }

        _GSize = { _GBounds.right - _GBounds.left, _GBounds.bottom - _GBounds.top };
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

    GetGaugeMetrics();

    // Render the gauge scales.
    {
        // Translate from client coordinates to element coordinates.
        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_XAxisBottom)
            {
                D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_GaugeNames.GetTextWidth() + 2, 0.f);

                renderTarget->SetTransform(Translate);
            }
        }
        else
        {
            if (_GraphSettings->_XAxisTop)
            {
                D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(0.f, _GaugeNames.GetTextHeight());

                renderTarget->SetTransform(Translate);
            }
        }

        _GaugeScales.Render(renderTarget);
    }

    // Render the gauges.
    {
        RenderGauges(renderTarget);
    }

    // Render the gauge names.
    {
        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_YAxisLeft)
            {
                D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(0.f, _GaugeScales.GetTextHeight() + (_GraphSettings->_FlipVertically ? -_GaugeMetrics._Offset : _GaugeMetrics._Offset));

                renderTarget->SetTransform(Translate);
            }
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
            {
                D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_GaugeScales.GetTextWidth() + (_GraphSettings->_FlipHorizontally ? -_GaugeMetrics._Offset : _GaugeMetrics._Offset), 0.f);

                renderTarget->SetTransform(Translate);
            }
        }

        _GaugeNames.Render(renderTarget, _GaugeMetrics);
    }

    ResetTransform(renderTarget);
}

/// <summary>
/// Gets the metrics used to render the gauges.
/// </summary>
void PeakMeter::GetGaugeMetrics() noexcept
{
    _GaugeMetrics._dBFSZero = Map(0., _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0., 1.);

    const FLOAT n = (FLOAT) _Analysis->_GaugeValues.size();

    _GaugeMetrics._TotalBarGap = _State->_GaugeGap * (FLOAT) (n - 1);
    _GaugeMetrics._TickSize = 4.f;
    _GaugeMetrics._TotalTickSize = (FLOAT) (_GraphSettings->_YAxisLeft + _GraphSettings->_YAxisRight) * _GaugeMetrics._TickSize;

    _GaugeMetrics._BarHeight = ::floor((_GSize.height - _GaugeMetrics._TotalBarGap - _GaugeMetrics._TotalTickSize) / n);
    _GaugeMetrics._BarWidth  = ::floor((_GSize.width  - _GaugeMetrics._TotalBarGap - _GaugeMetrics._TotalTickSize) / n);

    _GaugeMetrics._TotalBarHeight = (_GaugeMetrics._BarHeight * n) + _GaugeMetrics._TotalBarGap;
    _GaugeMetrics._TotalBarWidth  = (_GaugeMetrics._BarWidth  * n) + _GaugeMetrics._TotalBarGap;

    _GaugeMetrics._Offset = _State->_HorizontalPeakMeter ? ::floor((_GSize.height - _GaugeMetrics._TotalBarHeight) / 2.f): ::floor((_GSize.width - _GaugeMetrics._TotalBarWidth) / 2.f);
}

/// <summary>
/// Draws the meters.
/// </summary>
void PeakMeter::RenderGauges(ID2D1RenderTarget * renderTarget) const noexcept
{
    if ((_Analysis->_GaugeValues.size() == 0) || (_GSize.width <= 0.f) || (_GSize.height <= 0.f))
        return;

    const FLOAT PeakThickness = _MaxPeakStyle->_Thickness / 2.f;

    auto OldAntialiasMode = renderTarget->GetAntialiasMode();

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    // Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
    SetTransform(renderTarget, _GBounds);

    if (_State->_HorizontalPeakMeter)
    {
        D2D1_RECT_F Rect = { 0.f, 0.f, 0.f, _GSize.height - _GaugeMetrics._Offset };

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.top = std::clamp(Rect.bottom - _GaugeMetrics._BarHeight, 0.f, _GSize.height);

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                Rect.right = Rect.left + _GSize.width;

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            #endif
            }

            // Draw the peak.
            {
                double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, _GaugeMetrics._dBFSZero) : Value.PeakRender;

                // Draw the foreground (Peak).
                if (_PeakStyle->IsEnabled())
                {
                    Rect.right = Rect.left + (FLOAT) (PeakRender * _GSize.width);

                #ifndef _DEBUG_RENDER
                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                #else
                #endif
                }

                // Draw the foreground (Peak, 0dBFS)
                if ((Value.PeakRender > _GaugeMetrics._dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.right = Rect.left + (FLOAT) (Value.PeakRender * _GSize.width);

                    FLOAT OldLeft = Rect.left;

                    Rect.left = (FLOAT) (_GaugeMetrics._dBFSZero * _GSize.width);

                #ifndef _DEBUG_RENDER
                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                #else
                #endif

                    Rect.left = OldLeft;
                }

                // Draw the foreground (Peak Top).
                if ((_State->_PeakMode != PeakMode::None) && (Value.MaxPeakRender > 0.) && _MaxPeakStyle->IsEnabled())
                {
                    FLOAT OldLeft = Rect.left;

                    Rect.left  =
                    Rect.right = (FLOAT) ( Value.MaxPeakRender * _GSize.width);

                    Rect.left  = ::ceil(std::clamp(Rect.left  - PeakThickness, 0.f, _GSize.width));
                    Rect.right = ::ceil(std::clamp(Rect.right + PeakThickness, 0.f, _GSize.width));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) Value.Opacity : _MaxPeakStyle->_Opacity;

                    _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                    renderTarget->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                    Rect.left = OldLeft;
                }
            }

            // Draw the RMS.
            {
                double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, _GaugeMetrics._dBFSZero) : Value.RMSRender;

                // Draw the foreground (RMS).
                if (_RMSStyle->IsEnabled())
                {
                    Rect.right = Rect.left + (FLOAT) (RMSRender * _GSize.width);

                #ifndef _DEBUG_RENDER
                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                #else
                    DrawDebugRectangle(renderTarget, { 0.f, Rect.top, _ClientSize.width, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Red));
                #endif
                }

                // Draw the foreground (RMS, 0dBFS)
                if ((Value.RMSRender > _GaugeMetrics._dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.right = Rect.left + (FLOAT) (Value.RMSRender * _GSize.width);

                    FLOAT OldLeft = Rect.left;

                    Rect.left = (FLOAT) (_GaugeMetrics._dBFSZero * _GSize.width);

                #ifndef _DEBUG_RENDER
                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                #else
                #endif

                    Rect.left = OldLeft;
                }
            }

            Rect.bottom = Rect.top - _State->_GaugeGap;
        }
    }
    else
    {
        D2D1_RECT_F Rect = { _GaugeMetrics._Offset, 0.f, 0.f, 0.f };

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.right = std::clamp(Rect.left + _GaugeMetrics._BarWidth, 0.f, _GSize.width);

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                Rect.bottom = Rect.top + _GSize.height;

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            #endif
            }

            // Draw the peak.
            {
                double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, _GaugeMetrics._dBFSZero) : Value.PeakRender;

                // Draw the foreground (Peak).
                if (_PeakStyle->IsEnabled())
                {
                    Rect.bottom = Rect.top + (FLOAT) (PeakRender * _GSize.height);

                #ifndef _DEBUG_RENDER
                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                #else
                #endif
                }

                // Draw the foreground (Peak, 0dBFS)
                if ((Value.PeakRender > _GaugeMetrics._dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.bottom = Rect.top + (FLOAT) (Value.PeakRender * _GSize.height);

                    FLOAT OldTop = Rect.top;

                    Rect.top = (FLOAT) (_GaugeMetrics._dBFSZero * _GSize.height);

                #ifndef _DEBUG_RENDER
                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                #else
                #endif

                    Rect.top = OldTop;
                }

                // Draw the foreground (Peak Top).
                if ((_State->_PeakMode != PeakMode::None) && (Value.MaxPeakRender > 0.) && _MaxPeakStyle->IsEnabled())
                {
                    FLOAT OldTop = Rect.top;

                    Rect.top    =
                    Rect.bottom = (FLOAT) ( Value.MaxPeakRender * _GSize.height);

                    Rect.top    = ::ceil(std::clamp(Rect.top    - PeakThickness, 0.f, _GSize.height));
                    Rect.bottom = ::ceil(std::clamp(Rect.bottom + PeakThickness, 0.f, _GSize.height));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) Value.Opacity : _MaxPeakStyle->_Opacity;

                    _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                    renderTarget->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                    Rect.top = OldTop;
                }
            }

            // Draw the RMS.
            {
                double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, _GaugeMetrics._dBFSZero) : Value.RMSRender;

                // Draw the foreground (RMS).
                if (_RMSStyle->IsEnabled())
                {
                    Rect.bottom = Rect.top + (FLOAT) (RMSRender * _GSize.height);

                #ifndef _DEBUG_RENDER
                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                #else
                    DrawDebugRectangle(renderTarget, { Rect.left, 0.f, Rect.right, _ClientSize.height }, D2D1::ColorF(D2D1::ColorF::Red));
                #endif
                }

                // Draw the foreground (RMS, 0dBFS)
                if ((Value.RMSRender > _GaugeMetrics._dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.bottom = Rect.top + (FLOAT) (Value.RMSRender * _GSize.height);

                    FLOAT OldTop = Rect.top;

                    Rect.top = (FLOAT) (_GaugeMetrics._dBFSZero * _GSize.height);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.top = OldTop;
                }
            }

            Rect.left = Rect.right + _State->_GaugeGap;
        }
    }

    ResetTransform(renderTarget);

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(OldAntialiasMode);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT PeakMeter::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = _GaugeNames.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _GaugeScales.CreateDeviceSpecificResources(renderTarget);

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

    if (SUCCEEDED(hr))
        Resize();

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT PeakMeter::CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(_Size.width, _Size.height), &rt);

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
                    for (FLOAT x = _State->_LEDGap; x < _Size.width; x += (_State->_LEDSize + _State->_LEDGap))
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDSize, _Size.height), Brush);
                }
                else
                {
                    for (FLOAT y = _State->_LEDGap; y < _Size.height; y += (_State->_LEDSize + _State->_LEDGap))
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
/// Releases the device specific resources.
/// </summary>
void PeakMeter::ReleaseDeviceSpecificResources() noexcept
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

    _GaugeScales.ReleaseDeviceSpecificResources();

    _GaugeNames.ReleaseDeviceSpecificResources();
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

            if (!_GraphSettings->_XAxisBottom && (x < 0.f))
                x = 0.f;
            else
            if (!_GraphSettings->_XAxisTop && ((x + _TextStyle->GetWidth()) > GetWidth()))
                x = GetWidth() - _TextStyle->GetWidth();

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

            if (!_GraphSettings->_XAxisTop && (y < 0.f))
                y = 0.f;
            else
            if (!_GraphSettings->_XAxisBottom && ((y + _TextStyle->GetHeight()) > GetHeight()))
                y = GetHeight() - _TextStyle->GetHeight();

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
    #ifndef _DEBUG_RENDER
        _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

        for (const Label & Iter : _Labels)
        {
            // Draw the vertical grid line.
            if (_LineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.Point1, Iter.Point2, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect1, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    
                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect2, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    #else
        if (_GraphSettings->_YAxisLeft)
            DrawDebugRectangle(renderTarget, { _Bounds.left + 1.f,      _Bounds.top + 1.f, _ClientRect.left - 1.f, _Bounds.bottom }, D2D1::ColorF(D2D1::ColorF::Orange));

        if (_GraphSettings->_YAxisRight)
            DrawDebugRectangle(renderTarget, { _ClientRect.right + 1.f, _Bounds.top + 1.f, _Bounds.right,          _Bounds.bottom }, D2D1::ColorF(D2D1::ColorF::Orange));
    #endif
    }
    else
    {
    #ifndef _DEBUG_RENDER
        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            if (_LineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.Point1, Iter.Point2, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                if (_GraphSettings->_YAxisLeft)
                {
                    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect1, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                if (_GraphSettings->_YAxisRight)
                {
                    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.Rect2, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }
            }
        }
    #else
        if (_GraphSettings->_YAxisLeft)
            DrawDebugRectangle(renderTarget, { _Bounds.left + 1.f,      _Bounds.top + 1.f, _ClientRect.left - 1.f, _Bounds.bottom }, D2D1::ColorF(D2D1::ColorF::Orange));

        if (_GraphSettings->_YAxisRight)
            DrawDebugRectangle(renderTarget, { _ClientRect.right + 1.f, _Bounds.top + 1.f, _Bounds.right,          _Bounds.bottom }, D2D1::ColorF(D2D1::ColorF::Orange));
    #endif
    }
}

HRESULT GaugeScales::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, Size, L"-999", &_TextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, Size, L"", &_LineStyle);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void GaugeScales::ReleaseDeviceSpecificResources() noexcept
{
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
        DrawHorizontalNames(renderTarget, gaugeMetrics);
    else
        DrawVerticalNames(renderTarget, gaugeMetrics);
}

/// <summary>
/// Draws the channel names. Note: Rendered in absolute coordinates! No transformation.
/// </summary>
void GaugeNames::DrawHorizontalNames(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
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
                Rect.right = _GraphSettings->_FlipHorizontally ? GetWidth() : _TextStyle->GetWidth();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
/*
        // Draw the RMS text display.
        if (_RMSTextStyle->IsEnabled())
        {
            _RMSTextStyle->SetHorizontalAlignment(_GraphSettings->_FlipHorizontally ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_TRAILING);
            _RMSTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            D2D1_RECT_F TextRect = Rect;

            TextRect.left  = _GraphSettings->_FlipHorizontally ? _GBounds.right - 2.f - _RMSTextStyle->_Width : _GBounds.left + 2.f;
            TextRect.right = TextRect.left + _RMSTextStyle->_Width;

        #ifndef _DEBUG_RENDER
            WCHAR Text[16];

            if (::isfinite(gv.RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.RMS);
            else
                ::wcscpy_s(Text, _countof(Text), L"-∞");

            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _RMSTextStyle->_TextFormat, TextRect, _RMSTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        #else
            DrawDebugRectangle(renderTarget, TextRect, D2D1::ColorF(D2D1::ColorF::Purple));
        #endif
        }
*/
        Rect.top += _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight - _State->_GaugeGap : gaugeMetrics._BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Draws the channel names.
/// </summary>
void GaugeNames::DrawVerticalNames(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
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
                Rect.bottom = _GraphSettings->_FlipVertically ? GetHeight() : _TextStyle->GetHeight();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_GraphSettings->_XAxisBottom)
            {
                Rect.top    = _GraphSettings->_FlipVertically ? 0.f : GetHeight() - _TextStyle->GetHeight();
                Rect.bottom = _GraphSettings->_FlipVertically ? _TextStyle->GetHeight() : GetHeight();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
/*
        // Draw the RMS text display.
        if (_RMSTextStyle->IsEnabled())
        {
            _RMSTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            _RMSTextStyle->SetVerticalAlignment(_GraphSettings->_FlipVertically ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            D2D1_RECT_F TextRect = Rect;

            TextRect.top    = _GraphSettings->_FlipVertically ? _GBounds.top                              : _GBounds.bottom - _RMSTextStyle->GetHeight();
            TextRect.bottom = _GraphSettings->_FlipVertically ? _GBounds.top + _RMSTextStyle->GetHeight() : _GBounds.bottom;

        #ifndef _DEBUG_RENDER
            WCHAR Text[16];

            if (::isfinite(gv.RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.RMS);
            else
                ::wcscpy_s(Text, _countof(Text), L"-∞");

            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _RMSTextStyle->_TextFormat, TextRect, _RMSTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        #else
            DrawDebugRectangle(renderTarget, TextRect, D2D1::ColorF(D2D1::ColorF::Purple));
        #endif
        }
*/
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

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeRMSLevelText, renderTarget, _Size, L"-999.9", &_RMSTextStyle);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void GaugeNames::ReleaseDeviceSpecificResources() noexcept
{
    if (_RMSTextStyle)
    {
        _RMSTextStyle->ReleaseDeviceSpecificResources();
        _RMSTextStyle = nullptr;
    }

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}

#pragma endregion

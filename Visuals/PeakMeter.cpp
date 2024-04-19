
/** $VER: PeakMeter.cpp (2024.04.19) P. Stuer - Represents a peak meter. **/

#include "framework.h"
#include "PeakMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

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
    _PeakScale.Initialize(state, settings, analysis);

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
    _PeakScale.SetBounds(rect);

    SetBounds(rect);

    _OpacityMask.Release();
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakMeter::Reset()
{
    _IsResized = true;

    _PeakScale.Reset();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void PeakMeter::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    _PeakScale.Resize();

    // Gauge metrics
    {
        _GBounds = _Bounds;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_XAxisBottom)
                if (_GraphSettings->_FlipHorizontally)
                    _GBounds.right -= _XTextStyle->_TextWidth;
                else
                    _GBounds.left  += _XTextStyle->_TextWidth;

            if (_GraphSettings->_XAxisTop)
                if (_GraphSettings->_FlipHorizontally)
                    _GBounds.left  += _XTextStyle->_TextWidth;
                else
                    _GBounds.right -= _XTextStyle->_TextWidth;

            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_YAxisRight)
                    _GBounds.top += _PeakScale.GetHeight();

                if (_GraphSettings->_YAxisLeft)
                    _GBounds.bottom -= _PeakScale.GetHeight();
            }
            else
            {
                if (_GraphSettings->_YAxisLeft)
                    _GBounds.top += _PeakScale.GetHeight();

                if (_GraphSettings->_YAxisRight)
                    _GBounds.bottom -= _PeakScale.GetHeight();
            }

            _RMSTextStyle->SetHorizontalAlignment(_GraphSettings->_FlipHorizontally ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_TRAILING);
            _RMSTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                _GBounds.left  += _PeakScale.GetWidth();

            if (_GraphSettings->_YAxisRight)
                _GBounds.right -= _PeakScale.GetWidth();

            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_XAxisBottom)
                    _GBounds.top += _XTextStyle->_TextHeight;

                if (_GraphSettings->_XAxisTop)
                    _GBounds.bottom -= _XTextStyle->_TextHeight;
            }
            else
            {
                if (_GraphSettings->_XAxisTop)
                    _GBounds.top += _XTextStyle->_TextHeight;

                if (_GraphSettings->_XAxisBottom)
                    _GBounds.bottom -= _XTextStyle->_TextHeight;
            }

            _RMSTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            _RMSTextStyle->SetVerticalAlignment(_GraphSettings->_FlipVertically ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }

        _GSize = { _GBounds.right - _GBounds.left, _GBounds.bottom - _GBounds.top };
    }

    // Names metrics
    {
        _NBounds = _Bounds;
        _NSize = { _NBounds.right - _NBounds.left, _NBounds.bottom - _NBounds.top };
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

    _dBFSZero = Map(0., _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0., 1.);

    const FLOAT n = (FLOAT) _Analysis->_GaugeValues.size();

    _TotalBarGap = _State->_GaugeGap * (FLOAT) (n - 1);
    _TickSize = 2.f;
    _TotalTickSize = (_GraphSettings->_YAxisLeft ? _TickSize : 0.f) + (_GraphSettings->_YAxisRight ? _TickSize : 0.f);

    _BarHeight = ::floor((_GSize.height - _TotalBarGap - _TotalTickSize) / n);
    _BarWidth  = ::floor((_GSize.width  - _TotalBarGap - _TotalTickSize) / n);

    _TotalBarHeight = (_BarHeight * n) + _TotalBarGap;
    _TotalBarWidth  = (_BarWidth  * n) + _TotalBarGap;

    _Offset = _State->_HorizontalPeakMeter ? ::floor((_GSize.height - _TotalBarHeight) / 2.f): ::floor((_GSize.width - _TotalBarWidth) / 2.f);

#ifdef _DEBUG_RENDER
    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    DrawDebugRectangle(renderTarget, { _Bounds.left + 1, _Bounds.top + 1, _Bounds.right, _Bounds.bottom }, D2D1::ColorF(D2D1::ColorF::Green));
#endif

    _PeakScale.Render(renderTarget);
    DrawGauges(renderTarget);

    if (_State->_HorizontalPeakMeter)
        DrawHorizontalNames(renderTarget);
    else
        DrawVerticalNames(renderTarget);
}

/// <summary>
/// Draws the meters.
/// </summary>
void PeakMeter::DrawGauges(ID2D1RenderTarget * renderTarget) const noexcept
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
        D2D1_RECT_F Rect = { 0.f, 0.f, 0.f, _GSize.height - _Offset };

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.top = std::clamp(Rect.bottom - _BarHeight, 0.f, _GSize.height);

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
                double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, _dBFSZero) : Value.PeakRender;

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
                if ((Value.PeakRender > _dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.right = Rect.left + (FLOAT) (Value.PeakRender * _GSize.width);

                    FLOAT OldLeft = Rect.left;

                    Rect.left = (FLOAT) (_dBFSZero * _GSize.width);

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
                double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, _dBFSZero) : Value.RMSRender;

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
                if ((Value.RMSRender > _dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.right = Rect.left + (FLOAT) (Value.RMSRender * _GSize.width);

                    FLOAT OldLeft = Rect.left;

                    Rect.left = (FLOAT) (_dBFSZero * _GSize.width);

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
        D2D1_RECT_F Rect = { _Offset, 0.f, 0.f, 0.f };

        for (auto & Value : _Analysis->_GaugeValues)
        {
            Rect.right = std::clamp(Rect.left + _BarWidth, 0.f, _GSize.width);

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
                double PeakRender = _Peak0dBStyle->IsEnabled() ? std::min(Value.PeakRender, _dBFSZero) : Value.PeakRender;

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
                if ((Value.PeakRender > _dBFSZero) && _Peak0dBStyle->IsEnabled())
                {
                    Rect.bottom = Rect.top + (FLOAT) (Value.PeakRender * _GSize.height);

                    FLOAT OldTop = Rect.top;

                    Rect.top = (FLOAT) (_dBFSZero * _GSize.height);

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
                double RMSRender = _RMS0dBStyle->IsEnabled() ? std::min(Value.RMSRender, _dBFSZero) : Value.RMSRender;

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
                if ((Value.RMSRender > _dBFSZero) && _RMS0dBStyle->IsEnabled())
                {
                    Rect.bottom = Rect.top + (FLOAT) (Value.RMSRender * _GSize.height);

                    FLOAT OldTop = Rect.top;

                    Rect.top = (FLOAT) (_dBFSZero * _GSize.height);

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
/// Draws the channel names. Note: Rendered in absolute coordinates! No transformation.
/// </summary>
void PeakMeter::DrawHorizontalNames(ID2D1RenderTarget * renderTarget) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.top = _GraphSettings->_FlipVertically ? _GBounds.bottom - _Offset: _GBounds.top + _Offset;

    const FLOAT dy = _GraphSettings->_FlipVertically ? -_BarHeight : _BarHeight;

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.bottom = std::clamp(Rect.top + dy, 0.f, _GBounds.bottom);

        if (_XTextStyle->IsEnabled())
        {
            if (_GraphSettings->_XAxisTop)
            {
                Rect.left  = _GraphSettings->_FlipHorizontally ? _Bounds.left : _GBounds.right;
                Rect.right = _GraphSettings->_FlipHorizontally ? _GBounds.left : _Bounds.right;

            #ifndef _DEBUG_RENDER
                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            #else
                DrawDebugRectangle(renderTarget, { Rect.left, Rect.top, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
            #endif
            }

            if (_GraphSettings->_XAxisBottom)
            {
                Rect.left  = _GraphSettings->_FlipHorizontally ? _GBounds.right : _Bounds.left;
                Rect.right = _GraphSettings->_FlipHorizontally ? _Bounds.right : _GBounds.left;

            #ifndef _DEBUG_RENDER
                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            #else
                DrawDebugRectangle(renderTarget, { Rect.left, Rect.top, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
            #endif
            }
        }

        // Draw the RMS text display.
        if (_RMSTextStyle->IsEnabled())
        {
            D2D1_RECT_F TextRect = Rect;

            TextRect.left  = _GraphSettings->_FlipHorizontally ? _GBounds.right - 2.f - _RMSTextStyle->_TextWidth : _GBounds.left + 2.f;
            TextRect.right = TextRect.left + _RMSTextStyle->_TextWidth;

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

        Rect.top += _GraphSettings->_FlipVertically ? -_BarHeight - _State->_GaugeGap : _BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Draws the channel names. Note: Rendered in absolute coordinates! No transformation.
/// </summary>
void PeakMeter::DrawVerticalNames(ID2D1RenderTarget * renderTarget) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.left = _GraphSettings->_FlipHorizontally ? _GBounds.right - _Offset: _GBounds.left + _Offset;

    const FLOAT dx = _GraphSettings->_FlipHorizontally ? -_BarWidth : _BarWidth;

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.right = std::clamp(Rect.left + dx, 0.f, _GBounds.right);

        if (_XTextStyle->IsEnabled())
        {
            if (_GraphSettings->_XAxisTop)
            {
                Rect.top    = _GraphSettings->_FlipVertically ? _GBounds.bottom : _Bounds.top;
                Rect.bottom = _GraphSettings->_FlipVertically ? _Bounds.bottom : _GBounds.top;

            #ifndef _DEBUG_RENDER
                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            #else
                DrawDebugRectangle(renderTarget, { Rect.left, Rect.top, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
            #endif
            }

            if (_GraphSettings->_XAxisBottom)
            {
                Rect.top    = _GraphSettings->_FlipVertically ? _Bounds.top : _GBounds.bottom;
                Rect.bottom = _GraphSettings->_FlipVertically ? _GBounds.top: _Bounds.bottom;

            #ifndef _DEBUG_RENDER
                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            #else
                DrawDebugRectangle(renderTarget, { Rect.left, Rect.top, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
            #endif
            }
        }

        // Draw the RMS text display.
        if (_RMSTextStyle->IsEnabled())
        {
            D2D1_RECT_F TextRect = Rect;

            TextRect.top    = _GraphSettings->_FlipVertically ? _GBounds.top                              : _GBounds.bottom - _RMSTextStyle->_TextHeight;
            TextRect.bottom = _GraphSettings->_FlipVertically ? _GBounds.top + _RMSTextStyle->_TextHeight : _GBounds.bottom;

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

        Rect.left += _GraphSettings->_FlipHorizontally ? -_BarWidth - _State->_GaugeGap : _BarWidth + _State->_GaugeGap;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT PeakMeter::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(renderTarget);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterBackground, renderTarget, _Size, L"", &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterPeakLevel, renderTarget, _Size, L"", &_PeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeter0dBPeakLevel, renderTarget, _Size, L"", &_Peak0dBStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterMaxPeakLevel, renderTarget, _Size, L"", &_MaxPeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterRMSLevel, renderTarget, _Size, L"", &_RMSStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeter0dBRMSLevel, renderTarget, _Size, L"", &_RMS0dBStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterRMSLevelText, renderTarget, _Size, L"-999.9", &_RMSTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, _Size, L"WWW", &_XTextStyle);

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
    if (_YLineStyle)
    {
        _YLineStyle->ReleaseDeviceSpecificResources();
        _YLineStyle = nullptr;
    }

    if (_YTextStyle)
    {
        _YTextStyle->ReleaseDeviceSpecificResources();
        _YTextStyle = nullptr;
    }

    if (_XTextStyle)
    {
        _XTextStyle->ReleaseDeviceSpecificResources();
        _XTextStyle = nullptr;
    }

    if (_RMSTextStyle)
    {
        _RMSTextStyle->ReleaseDeviceSpecificResources();
        _RMSTextStyle = nullptr;
    }

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

#pragma region Scale

/// <summary>
/// Initializes this instance.
/// </summary>
void PeakScale::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
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
void PeakScale::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakScale::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void PeakScale::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    if (_State->_HorizontalPeakMeter)
    {
        const FLOAT cx = (_TextStyle->_TextWidth / 2.f);

        // Calculate the position of the labels based on the width.
        D2D1_RECT_F OldRect = {  };

        const FLOAT xMin = !_GraphSettings->_FlipHorizontally ? _GBounds.left : _GBounds.right;
        const FLOAT xMax = !_GraphSettings->_FlipHorizontally ? _GBounds.right : _GBounds.left;

        for (Label & Iter : _Labels)
        {
            FLOAT x = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., xMin, xMax);

            // Don't generate any labels outside the bounds.
            if (!InRange(x, _GBounds.left, _GBounds.right))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.PointL = D2D1_POINT_2F(x, _GBounds.top);
            Iter.PointR = D2D1_POINT_2F(x, _GBounds.bottom);

            if (_GraphSettings->_FlipHorizontally)
                x = std::clamp(x - cx, _GraphSettings->_XAxisTop    ? _GBounds.left - cx : _Bounds.left + 1.f, _GraphSettings->_XAxisBottom ? _GBounds.right + cx : _GBounds.right - _TextStyle->_TextWidth);
            else
                x = std::clamp(x - cx, _GraphSettings->_XAxisBottom ? _GBounds.left - cx : _Bounds.left + 1.f, _GraphSettings->_XAxisTop    ? _GBounds.right + cx : _GBounds.right - _TextStyle->_TextWidth);

            if (_GraphSettings->_FlipVertically)
            {
                Iter.RectL = { x, _GBounds.bottom, x + _TextStyle->_TextWidth, _Bounds.bottom };
                Iter.RectR = { x, _Bounds.top,     x + _TextStyle->_TextWidth, _GBounds.top };
            }
            else
            {
                Iter.RectL = { x, _Bounds.top,     x + _TextStyle->_TextWidth, _GBounds.top };
                Iter.RectR = { x, _GBounds.bottom, x + _TextStyle->_TextWidth, _Bounds.bottom };
            }

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingHorizontally(Iter.RectL, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.RectL;
        }
    }
    else
    {
        const FLOAT cy = (_TextStyle->_TextHeight / 2.f);

        // Calculate the position of the labels based on the height.
        D2D1_RECT_F OldRect = {  };

        const FLOAT yMin = !_GraphSettings->_FlipVertically ? _GBounds.bottom : _GBounds.top;
        const FLOAT yMax = !_GraphSettings->_FlipVertically ? _GBounds.top : _GBounds.bottom;

        for (Label & Iter : _Labels)
        {
            FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., yMin, yMax);

            // Don't generate any labels outside the bounds.
            if (!InRange(y, _GBounds.top, _GBounds.bottom))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.PointL = D2D1_POINT_2F(_GBounds.left,  y);
            Iter.PointR = D2D1_POINT_2F(_GBounds.right, y);

            if (_GraphSettings->_FlipVertically)
                y = std::clamp(y - cy, _GraphSettings->_XAxisBottom ? _GBounds.top - cy : _Bounds.top + 1.f, _GraphSettings->_XAxisTop    ? _GBounds.bottom + cy : _Bounds.bottom - _TextStyle->_TextHeight);
            else
                y = std::clamp(y - cy, _GraphSettings->_XAxisTop    ? _GBounds.top - cy : _Bounds.top + 1.f, _GraphSettings->_XAxisBottom ? _GBounds.bottom + cy : _Bounds.bottom - _TextStyle->_TextHeight);

            Iter.RectL = { _Bounds.left,      y, _GBounds.left, y + _TextStyle->_TextHeight };
            Iter.RectR = { _GBounds.right, y, _Bounds.right   , y + _TextStyle->_TextHeight };

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingVertically(Iter.RectL, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.RectL;
        }
    }
}

/// <summary>
/// Renders this instance.
/// </summary>
void PeakScale::Render(ID2D1RenderTarget * renderTarget)
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
                renderTarget->DrawLine(Iter.PointL, Iter.PointR, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectL, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    
                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectR, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
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
                renderTarget->DrawLine(Iter.PointL, Iter.PointR, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _TextStyle->IsEnabled())
            {
                if (_GraphSettings->_YAxisLeft)
                {
                    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectL, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                if (_GraphSettings->_YAxisRight)
                {
                    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectR, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
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

HRESULT PeakScale::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, _Size, L"-999", &_TextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, _Size, L"", &_LineStyle);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void PeakScale::ReleaseDeviceSpecificResources() noexcept
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

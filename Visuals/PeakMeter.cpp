
/** $VER: PeakMeter.cpp (2024.04.09) P. Stuer - Represents a peak meter. **/

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
/// Moves this instance on the canvas.
/// </summary>
void PeakMeter::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);

    _OpacityMask.Release();
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakMeter::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void PeakMeter::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    _ClientRect = _Bounds;

    if (_State->_HorizontalPeakMeter)
    {
        {
            if (_GraphSettings->_XAxisBottom)
                if (_GraphSettings->_FlipHorizontally)
                    _ClientRect.right -= _XTextStyle->_TextWidth;
                else
                    _ClientRect.left  += _XTextStyle->_TextWidth;

            if (_GraphSettings->_XAxisTop)
                if (_GraphSettings->_FlipHorizontally)
                    _ClientRect.left  += _XTextStyle->_TextWidth;
                else
                    _ClientRect.right -= _XTextStyle->_TextWidth;

            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_YAxisRight)
                    _ClientRect.top += _YTextStyle->_TextHeight;

                if (_GraphSettings->_YAxisLeft)
                    _ClientRect.bottom -= _YTextStyle->_TextHeight;
            }
            else
            {
                if (_GraphSettings->_YAxisLeft)
                    _ClientRect.top += _YTextStyle->_TextHeight;

                if (_GraphSettings->_YAxisRight)
                    _ClientRect.bottom -= _YTextStyle->_TextHeight;
            }

            _RMSTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            _RMSTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        const FLOAT cx = (_YTextStyle->_TextWidth / 2.f);

        // Calculate the position of the labels based on the width.
        D2D1_RECT_F OldRect = {  };

        for (Label & Iter : _Labels)
        {
            FLOAT x = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipHorizontally ? _ClientRect.left : _ClientRect.right, !_GraphSettings->_FlipHorizontally ? _ClientRect.right : _ClientRect.left);

            // Don't generate any labels outside the bounds.
            if (!InRange(x, _ClientRect.left, _ClientRect.right))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.PointL = D2D1_POINT_2F(x, _ClientRect.top);
            Iter.PointR = D2D1_POINT_2F(x, _ClientRect.bottom);

            if (_GraphSettings->_FlipHorizontally)
                x = Clamp(x - cx, _GraphSettings->_XAxisTop    ? _ClientRect.left - cx : _Bounds.left + 1.f, _GraphSettings->_XAxisBottom ? _ClientRect.right + cx : _ClientRect.right - _YTextStyle->_TextWidth);
            else
                x = Clamp(x - cx, _GraphSettings->_XAxisBottom ? _ClientRect.left - cx : _Bounds.left + 1.f, _GraphSettings->_XAxisTop    ? _ClientRect.right + cx : _ClientRect.right - _YTextStyle->_TextWidth);

            if (_GraphSettings->_FlipVertically)
            {
                Iter.RectL = { x, _ClientRect.bottom, x + _YTextStyle->_TextWidth, _Bounds.bottom };
                Iter.RectR = { x, _Bounds.top,        x + _YTextStyle->_TextWidth, _ClientRect.top };
            }
            else
            {
                Iter.RectL = { x, _Bounds.top,        x + _YTextStyle->_TextWidth, _ClientRect.top };
                Iter.RectR = { x, _ClientRect.bottom, x + _YTextStyle->_TextWidth, _Bounds.bottom };
            }

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingHorizontally(Iter.RectL, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.RectL;
        }
    }
    else
    {
        {
            if (_GraphSettings->_YAxisLeft)
                _ClientRect.left  += _YTextStyle->_TextWidth;

            if (_GraphSettings->_YAxisRight)
                _ClientRect.right -= _YTextStyle->_TextWidth;

            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_XAxisBottom)
                    _ClientRect.top += _XTextStyle->_TextHeight;

                if (_GraphSettings->_XAxisTop)
                    _ClientRect.bottom -= _XTextStyle->_TextHeight;
            }
            else
            {
                if (_GraphSettings->_XAxisTop)
                    _ClientRect.top += _XTextStyle->_TextHeight;

                if (_GraphSettings->_XAxisBottom)
                    _ClientRect.bottom -= _XTextStyle->_TextHeight;
            }

            _RMSTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            _RMSTextStyle->SetVerticalAlignment(_GraphSettings->_FlipVertically ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR : DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        }

        const FLOAT cy = (_YTextStyle->_TextHeight / 2.f);

        // Calculate the position of the labels based on the height.
        D2D1_RECT_F OldRect = {  };

        for (Label & Iter : _Labels)
        {
            FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipVertically ? _ClientRect.bottom : _ClientRect.top, !_GraphSettings->_FlipVertically ? _ClientRect.top : _ClientRect.bottom);

            // Don't generate any labels outside the bounds.
            if (!InRange(y, _ClientRect.top, _ClientRect.bottom))
            {
                Iter.IsHidden = true;
                continue;
            }

            Iter.PointL = D2D1_POINT_2F(_ClientRect.left,  y);
            Iter.PointR = D2D1_POINT_2F(_ClientRect.right, y);

            if (_GraphSettings->_FlipVertically)
                y = Clamp(y - cy, _GraphSettings->_XAxisBottom ? _ClientRect.top - cy : _Bounds.top + 1.f, _GraphSettings->_XAxisTop    ? _ClientRect.bottom + cy : _Bounds.bottom - _YTextStyle->_TextHeight);
            else
                y = Clamp(y - cy, _GraphSettings->_XAxisTop    ? _ClientRect.top - cy : _Bounds.top + 1.f, _GraphSettings->_XAxisBottom ? _ClientRect.bottom + cy : _Bounds.bottom - _YTextStyle->_TextHeight);

            Iter.RectL = { _Bounds.left,      y, _ClientRect.left, y + _YTextStyle->_TextHeight };
            Iter.RectR = { _ClientRect.right, y, _Bounds.right   , y + _YTextStyle->_TextHeight };

            // Hide overlapping labels except for the first and the last one.
            Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingVertically(Iter.RectL, OldRect);

            if (!Iter.IsHidden)
                OldRect = Iter.RectL;
        }
    }

    _ClientSize = { _ClientRect.right - _ClientRect.left, _ClientRect.bottom - _ClientRect.top };

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

    _ZeroDecibel = Map(0., _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0., 1.);

#ifdef _DEBUG_RENDER
    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    DrawDebugRectangle(renderTarget, { _Bounds.left + 1, _Bounds.top + 1, _Bounds.right, _Bounds.bottom }, D2D1::ColorF(D2D1::ColorF::Green));
#endif

    DrawScale(renderTarget);
    DrawMeters(renderTarget);
}

/// <summary>
/// Draws the scale.
/// </summary>
void PeakMeter::DrawScale(ID2D1RenderTarget * renderTarget) const noexcept
{
    if ((_GraphSettings->_YAxisMode == YAxisMode::None) || (!_GraphSettings->_YAxisLeft && !_GraphSettings->_YAxisRight))
        return;

    if (_State->_HorizontalPeakMeter)
    {
    #ifndef _DEBUG_RENDER
        _YTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            if (_YLineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.PointL, Iter.PointR, _YLineStyle->_Brush, _YLineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _YTextStyle->IsEnabled())
            {
                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectL, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    
                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectR, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
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
            if (_YLineStyle->IsEnabled())
                renderTarget->DrawLine(Iter.PointL, Iter.PointR, _YLineStyle->_Brush, _YLineStyle->_Thickness, nullptr);

            // Draw the text.
            if (!Iter.IsHidden && _YTextStyle->IsEnabled())
            {
                // Draw the labels.
                if (_GraphSettings->_YAxisLeft)
                {
                    _YTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectL, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                if (_GraphSettings->_YAxisRight)
                {
                    _YTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectR, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
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

/// <summary>
/// Draws the meters.
/// </summary>
void PeakMeter::DrawMeters(ID2D1RenderTarget * renderTarget) const noexcept
{
    if ((_Analysis->_MeterValues.size() == 0) || (_ClientSize.width <= 0.f) || (_ClientSize.height <= 0.f))
        return;

    const FLOAT PeakThickness = _MaxPeakStyle->_Thickness / 2.f;

    auto OldAntialiasMode = renderTarget->GetAntialiasMode();

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    const FLOAT n = (FLOAT) _Analysis->_MeterValues.size();
    const FLOAT BarGap = 1.f;
    const FLOAT TotalBarGap = BarGap * (n - 1);
    const FLOAT TickSize = 2.f;
    const FLOAT TotalTickSize = (_GraphSettings->_YAxisLeft ? TickSize : 0.f) + (_GraphSettings->_YAxisRight ? TickSize : 0.f);

    if (_State->_HorizontalPeakMeter)
    {
        SetTransform(renderTarget, _ClientRect);

        const FLOAT BarHeight = ::floor((_ClientSize.height - TotalBarGap - TotalTickSize) / n);
        const FLOAT TotalBarHeight = (BarHeight * n) + TotalBarGap;
        const FLOAT Offset = ::floor((_ClientSize.height - TotalBarHeight) / 2.f);

        D2D1_RECT_F Rect = { 0.f, (_ClientSize.height - 1.f) - Offset - BarHeight, 0.f, 0.f };

        for (auto & mv : _Analysis->_MeterValues)
        {
            // FIXME: Ugly hack. FillOpacityMask() does not render when the top coordinate is odd.
            if ((int) Rect.top & 1)
                Rect.top--;

            Rect.bottom = Clamp(Rect.top + BarHeight, 0.f, _ClientSize.height - 1.f);

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                Rect.right = _ClientSize.width;

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            #endif
            }

            // Draw the foreground (Peak).
            if (_PeakStyle->IsEnabled())
            {
                Rect.right = (FLOAT) (mv.SmoothedPeak * _ClientSize.width);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                // Draw the foreground (Peak, 0dB)
                if ((mv.SmoothedPeak > _ZeroDecibel) && _Peak0dBStyle->IsEnabled())
                {
                    FLOAT OldLeft = Rect.left;

                    Rect.left = (FLOAT) (_ZeroDecibel * _ClientSize.width);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.left = OldLeft;
                }
            #else
            #endif
            }

            // Draw the foreground (Peak Top).
            if ((_State->_PeakMode != PeakMode::None) && (mv.MaxSmoothedPeak > 0.) && _MaxPeakStyle->IsEnabled())
            {
                FLOAT OldLeft = Rect.left;

                Rect.left  =
                Rect.right = (FLOAT) ( mv.MaxSmoothedPeak * _ClientSize.width);

                Rect.left  = ::ceil(Clamp(Rect.left  - PeakThickness, 0.f, _ClientSize.width));
                Rect.right = ::ceil(Clamp(Rect.right + PeakThickness, 0.f, _ClientSize.width));

                FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) mv.Opacity : _MaxPeakStyle->_Opacity;

                _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                renderTarget->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                Rect.left = OldLeft;
            }

            // Draw the foreground (RMS).
            if (_RMSStyle->IsEnabled())
            {
                Rect.right = (FLOAT) (mv.SmoothedRMS * _ClientSize.width);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                if ((mv.SmoothedRMS > _ZeroDecibel) && _RMS0dBStyle->IsEnabled())
                {
                    FLOAT OldLeft = Rect.left;

                    Rect.left = (FLOAT) (_ZeroDecibel * _ClientSize.width);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.left = OldLeft;
                }
            #else
                DrawDebugRectangle(renderTarget, { 0.f, Rect.top, _ClientSize.width, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Red));
            #endif
            }

            Rect.top -= BarGap + BarHeight;
        }

        ResetTransform(renderTarget);

        // Draw the channel names.
        if (_XTextStyle->IsEnabled() && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.top = _GraphSettings->_FlipVertically ? _ClientRect.bottom : _ClientRect.top;

            const FLOAT dy = _GraphSettings->_FlipVertically ? -(_ClientSize.height / n) : (_ClientSize.height / n);

            for (const auto & mv : _Analysis->_MeterValues)
            {
                Rect.bottom = Clamp(Rect.top + dy, 0.f, _ClientRect.bottom);

                if (_GraphSettings->_XAxisTop)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? _Bounds.left : _ClientRect.right + 1.f;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _ClientRect.left - 1.f : _Bounds.right;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                    DrawDebugRectangle(renderTarget, { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
                #endif
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? _ClientRect.right + 1.f : _Bounds.left;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _Bounds.right : _ClientRect.left - 1.f;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                    DrawDebugRectangle(renderTarget, { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
                #endif

                    // Draw the RMS text display.
                    if (_RMSTextStyle->IsEnabled() && ::isfinite(mv.RMS))
                    {
                        D2D1_RECT_F TextRect = Rect;

                        TextRect.left  = _GraphSettings->_FlipHorizontally ? _ClientRect.right - 2.f - _RMSTextStyle->_TextWidth : _ClientRect.left + 2.f;
                        TextRect.right = TextRect.left + _RMSTextStyle->_TextWidth;

                    #ifndef _DEBUG_RENDER
                        WCHAR Text[16]; ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", mv.RMS);

                        renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _RMSTextStyle->_TextFormat, TextRect, _RMSTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
                    #else
                        DrawDebugRectangle(renderTarget, TextRect, D2D1::ColorF(D2D1::ColorF::Purple));
                    #endif
                    }
                }

                Rect.top = Rect.bottom + 1.f;
            }
        }
    }
    else
    {
        SetTransform(renderTarget, _ClientRect);

        const FLOAT BarWidth = ::floor((_ClientSize.width - TotalBarGap - TotalTickSize) / n);
        const FLOAT TotalBarWidth = (BarWidth * n) + TotalBarGap;
        const FLOAT Offset = ::floor((_ClientSize.width - TotalBarWidth) / 2.f);

        D2D1_RECT_F Rect = { Offset, 0.f, 0.f, 0.f };

        for (auto & mv : _Analysis->_MeterValues)
        {
            // FIXME: Ugly hack. FillOpacityMask() does not render when the top coordinate is odd.
            if ((int) Rect.left & 1)
                Rect.left++;

            Rect.right = Clamp(Rect.left + BarWidth, 0.f, _ClientSize.width - 1.f);

            // Draw the background.
            if (_BackgroundStyle->IsEnabled())
            {
                Rect.bottom = _ClientSize.height;

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            #endif
            }

            // Draw the foreground (Peak).
            if (_PeakStyle->IsEnabled())
            {
                Rect.bottom = (FLOAT) (mv.SmoothedPeak * _ClientSize.height);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                // Draw the foreground (Peak, 0dB)
                if ((mv.SmoothedPeak > _ZeroDecibel) && _Peak0dBStyle->IsEnabled())
                {
                    FLOAT OldTop = Rect.top;

                    Rect.top = (FLOAT) (_ZeroDecibel * _ClientSize.height);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _Peak0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _Peak0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.top = OldTop;
                }
            #else
            #endif
            }

            // Draw the foreground (Peak Top).
            if ((_State->_PeakMode != PeakMode::None) && (mv.MaxSmoothedPeak > 0.) && _MaxPeakStyle->IsEnabled())
            {
                FLOAT OldTop = Rect.top;

                Rect.top    =
                Rect.bottom = (FLOAT) ( mv.MaxSmoothedPeak * _ClientSize.height);

                Rect.top    = ::ceil(Clamp(Rect.top    - PeakThickness, 0.f, _ClientSize.height));
                Rect.bottom = ::ceil(Clamp(Rect.bottom + PeakThickness, 0.f, _ClientSize.height));

                FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) mv.Opacity : _MaxPeakStyle->_Opacity;

                _MaxPeakStyle->_Brush->SetOpacity(Opacity);

                renderTarget->FillRectangle(Rect, _MaxPeakStyle->_Brush);

                Rect.top = OldTop;
            }

            // Draw the foreground (RMS).
            if (_RMSStyle->IsEnabled())
            {
                Rect.bottom = (FLOAT) (mv.SmoothedRMS * _ClientSize.height);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                if ((mv.SmoothedRMS > _ZeroDecibel) && _RMS0dBStyle->IsEnabled())
                {
                    FLOAT OldTop = Rect.top;

                    Rect.top = (FLOAT) (_ZeroDecibel * _ClientSize.height);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _RMS0dBStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _RMS0dBStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);

                    Rect.top = OldTop;
                }
            #else
                DrawDebugRectangle(renderTarget, { Rect.left, 0.f, Rect.right, _ClientSize.height }, D2D1::ColorF(D2D1::ColorF::Red));
            #endif
            }

            Rect.left = Rect.right + BarGap;
        }

    #ifdef _DEBUG_RENDER
        DrawDebugRectangle(renderTarget, { 0.f, 0.f, _ClientSize.width, _ClientSize.height }, D2D1::ColorF(D2D1::ColorF::Pink));
    #endif

        ResetTransform(renderTarget);

        // Draw the channel names.
        if (_XTextStyle->IsEnabled() && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.left = _GraphSettings->_FlipHorizontally ? _ClientRect.right : _ClientRect.left;

            const FLOAT dx = _GraphSettings->_FlipHorizontally ? -(_ClientSize.width / n) : (_ClientSize.width / n);

            for (const auto & mv : _Analysis->_MeterValues)
            {
                Rect.right = Clamp(Rect.left + dx, 0.f, _ClientRect.right);

                if (_GraphSettings->_XAxisTop)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? _ClientRect.bottom + 1.f : _Bounds.top;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _Bounds.bottom : _ClientRect.top - 1.f;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                    DrawDebugRectangle(renderTarget, { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
                #endif
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? _Bounds.top : _ClientRect.bottom + 1.f;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _ClientRect.top - 1.f: _Bounds.bottom;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                    DrawDebugRectangle(renderTarget, { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom }, D2D1::ColorF(D2D1::ColorF::Blue));
                #endif

                    // Draw the RMS text display.
                    if (_RMSTextStyle->IsEnabled() && ::isfinite(mv.RMS))
                    {
                        D2D1_RECT_F TextRect = Rect;

                        TextRect.top    = _GraphSettings->_FlipVertically ? _ClientRect.top + 1.f : Rect.top  - (_RMSTextStyle->_TextHeight + 1.f);
                        TextRect.bottom = _GraphSettings->_FlipVertically ? TextRect.top + (_RMSTextStyle->_TextHeight + 1.f) : Rect.top;

                    #ifndef _DEBUG_RENDER
                        WCHAR Text[16]; ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", mv.RMS);

                        renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _RMSTextStyle->_TextFormat, TextRect, _RMSTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
                    #else
                        DrawDebugRectangle(renderTarget, TextRect, D2D1::ColorF(D2D1::ColorF::Purple));
                    #endif
                    }
                }

                Rect.left = Rect.right + 1.f;
            }
        }
    }

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(OldAntialiasMode);
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
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, _Size, L"-999", &_YTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, _Size, L"", &_YLineStyle);

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

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(_State->_HorizontalPeakMeter ? D2D1::SizeF(_Size.width, 1.f) : D2D1::SizeF(1.f, _Size.height), &rt);

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
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDSize, 1.f), Brush);
                }
                else
                {
                    for (FLOAT y = _State->_LEDGap; y < _Size.height; y += (_State->_LEDSize + _State->_LEDGap))
                        rt->FillRectangle(D2D1::RectF(0.f, y, 1.f, y + _State->_LEDSize), Brush);
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

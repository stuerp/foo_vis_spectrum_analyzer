
/** $VER: PeakMeter.cpp (2024.04.05) P. Stuer - Represents a peak meter. **/

#include "PeakMeter.h"

#include "Support.h"

#include "DirectWrite.h"

#pragma hdrstop

//#define _DEBUG_RENDER

PeakMeter::PeakMeter()
{
    _Bounds = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void PeakMeter::Initialize(State * state, const GraphSettings * settings)
{
    _State = state;
    _GraphSettings = settings;

    ReleaseDeviceSpecificResources();

    _Labels.clear();

    if (_GraphSettings->_YAxisMode == YAxisMode::None)
        return;

    // Create the labels.
    {
        for (double Amplitude = _GraphSettings->_AmplitudeLo; Amplitude <= _GraphSettings->_AmplitudeHi; Amplitude -= _GraphSettings->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%d", (int) Amplitude);

            Label lb = { Amplitude, Text };

            _Labels.push_back(lb);
        }
    }
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void PeakMeter::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;
    _Size = { rect.right - rect.left, rect.bottom - rect.top };

    _OpacityMask.Release();

    _IsResized = true;
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
    if (!_IsResized)
        return;

    _ClientRect = _Bounds;

    if (_State->_HorizontalPeakMeter)
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
    }
    else
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
    }

    if (_State->_HorizontalPeakMeter)
    {
        const FLOAT cx = (_YTextStyle->_TextWidth / 2.f);

        // Calculate the position of the labels based on the width.
        for (Label & Iter : _Labels)
        {
            FLOAT x = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipHorizontally ? _ClientRect.left : _ClientRect.right, !_GraphSettings->_FlipHorizontally ? _ClientRect.right : _ClientRect.left);

            // Don't generate any labels outside the bounds.
            if (!InRange(x, _ClientRect.left, _ClientRect.right))
                continue;

            Iter.PointL = D2D1_POINT_2F(x, _ClientRect.top);
            Iter.PointR = D2D1_POINT_2F(x, _ClientRect.bottom);

            if (_GraphSettings->_FlipHorizontally)
                x = Clamp(x - cx, _GraphSettings->_XAxisTop    ? _ClientRect.left - cx : _Bounds.left + 1.f, _GraphSettings->_XAxisBottom ? _ClientRect.right + cx : _ClientRect.right - _YTextStyle->_TextWidth);
            else
                x = Clamp(x - cx, _GraphSettings->_XAxisBottom ? _ClientRect.left - cx : _Bounds.left + 1.f, _GraphSettings->_XAxisTop    ? _ClientRect.right + cx : _ClientRect.right - _YTextStyle->_TextWidth);

            if (_GraphSettings->_FlipVertically)
                Iter.RectL = { x, _ClientRect.bottom, x + _YTextStyle->_TextWidth, _Bounds.bottom };
            else
                Iter.RectL = { x, _Bounds.top,        x + _YTextStyle->_TextWidth, _ClientRect.top };

            if (_GraphSettings->_FlipVertically)
                Iter.RectR = { x, _Bounds.top,        x + _YTextStyle->_TextWidth, _ClientRect.top };
            else
                Iter.RectR = { x, _ClientRect.bottom, x + _YTextStyle->_TextWidth, _Bounds.bottom };
        }
    }
    else
    {
        const FLOAT cy = (_YTextStyle->_TextHeight / 2.f);

        // Calculate the position of the labels based on the height.
        for (Label & Iter : _Labels)
        {
            FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipVertically ? _ClientRect.bottom : _ClientRect.top, !_GraphSettings->_FlipVertically ? _ClientRect.top : _ClientRect.bottom);

            // Don't generate any labels outside the bounds.
            if (!InRange(y, _ClientRect.top, _ClientRect.bottom))
                continue;

            Iter.PointL = D2D1_POINT_2F(_ClientRect.left,  y);
            Iter.PointR = D2D1_POINT_2F(_ClientRect.right, y);

            if (_GraphSettings->_FlipVertically)
                y = Clamp(y - cy, _GraphSettings->_XAxisBottom ? _ClientRect.top - cy : _Bounds.top + 1.f, _GraphSettings->_XAxisTop    ? _ClientRect.bottom + cy : _Bounds.bottom - _YTextStyle->_TextHeight);
            else
                y = Clamp(y - cy, _GraphSettings->_XAxisTop    ? _ClientRect.top - cy : _Bounds.top + 1.f, _GraphSettings->_XAxisBottom ? _ClientRect.bottom + cy : _Bounds.bottom - _YTextStyle->_TextHeight);

            Iter.RectL = { _Bounds.left,      y, _ClientRect.left, y + _YTextStyle->_TextHeight };
            Iter.RectR = { _ClientRect.right, y, _Bounds.right   , y + _YTextStyle->_TextHeight };
        }
    }

    _ClientSize = { _ClientRect.right - _ClientRect.left, _ClientRect.bottom - _ClientRect.top };

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void PeakMeter::Render(ID2D1RenderTarget * renderTarget, Analysis & analysis)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

#ifdef _DEBUG_RENDER
    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    D2D1_RECT_F DebugRect = { _Bounds.left + 1, _Bounds.top + 1, _Bounds.right, _Bounds.bottom };
    _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
#endif

    DrawScale(renderTarget);
    DrawMeters(renderTarget, analysis);
}

/// <summary>
/// Draws the scale.
/// </summary>
void PeakMeter::DrawScale(ID2D1RenderTarget * renderTarget) const noexcept
{
    if ((_GraphSettings->_YAxisMode == YAxisMode::None) || (!_GraphSettings->_YAxisLeft && !_GraphSettings->_YAxisRight))
        return;

    D2D1_RECT_F OldRect = {  };

    if (_State->_HorizontalPeakMeter)
    {
    #ifndef _DEBUG_RENDER
        _YTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            if (_YLineStyle->_ColorSource != ColorSource::None)
                renderTarget->DrawLine(Iter.PointL, Iter.PointR, _YLineStyle->_Brush, _YLineStyle->_Thickness, nullptr);

            // Prevent overdraw of the labels.
            if ((_YTextStyle->_ColorSource != ColorSource::None) && !InRange(Iter.RectL.left, OldRect.left, OldRect.right) && !InRange(Iter.RectL.left, OldRect.left, OldRect.right))
            {
                // Draw the labels.
                if (_GraphSettings->_YAxisLeft)
                {
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectL, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }
                    
                if (_GraphSettings->_YAxisRight)
                {
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectR, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                OldRect = Iter.RectL;
            }
        }
    #else
        if (_GraphSettings->_YAxisLeft)
        {
            D2D1_RECT_F DebugRect = { _Bounds.left + 1.f, _Bounds.top + 1.f, _ClientRect.left - 1.f, _Bounds.bottom };
        
            _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Orange)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
        }
        if (_GraphSettings->_YAxisRight)
        {
            D2D1_RECT_F DebugRect = { _ClientRect.right + 1.f, _Bounds.top + 1.f, _Bounds.right, _Bounds.bottom };
        
            _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Orange)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
        }
    #endif
    }
    else
    {
    #ifndef _DEBUG_RENDER
        for (const Label & Iter : _Labels)
        {
            // Draw the horizontal grid line.
            if (_YLineStyle->_ColorSource != ColorSource::None)
                renderTarget->DrawLine(Iter.PointL, Iter.PointR, _YLineStyle->_Brush, _YLineStyle->_Thickness, nullptr);

            // Prevent overdraw of the labels.
            if ((_YTextStyle->_ColorSource != ColorSource::None) && !InRange(Iter.RectL.top, OldRect.top, OldRect.bottom) && !InRange(Iter.RectL.bottom, OldRect.top, OldRect.bottom))
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

                OldRect = Iter.RectL;
            }
        }
    #else
        if (_GraphSettings->_YAxisLeft)
        {
            D2D1_RECT_F DebugRect = { _Bounds.left + 1.f, _Bounds.top + 1.f, _ClientRect.left - 1.f, _Bounds.bottom };
        
            _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Orange)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
        }
        if (_GraphSettings->_YAxisRight)
        {
            D2D1_RECT_F DebugRect = { _ClientRect.right + 1.f, _Bounds.top + 1.f, _Bounds.right, _Bounds.bottom };
        
            _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Orange)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
        }
    #endif
    }
}

/// <summary>
/// Draws the meters.
/// </summary>
void PeakMeter::DrawMeters(ID2D1RenderTarget * renderTarget, Analysis & analysis) const noexcept
{
    if (analysis._MeterValues.size() == 0)
        return;

    auto OldAntialiasMode = renderTarget->GetAntialiasMode();

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    const FLOAT n = (FLOAT) analysis._MeterValues.size();
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

        for (auto & mv : analysis._MeterValues)
        {
            Rect.bottom = Clamp(Rect.top + BarHeight, 0.f, _ClientSize.height - 1.f);

            // Draw the background.
            if (_BackgroundStyle->_ColorSource != ColorSource::None)
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

            // Draw the foreground.
            if (_PeakStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledPeak, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.right = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0.f, _ClientSize.width);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            #endif
            }

            if (_RMSStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledRMS, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.right = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0.f, _ClientSize.width);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            {
                D2D1_RECT_F DebugRect = { 0.f, Rect.top, _ClientSize.width, Rect.bottom };

                _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
            }
            #endif
            }

            // Animate the scaled peak and RMS values.
            mv.ScaledPeak = Clamp(mv.ScaledPeak - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            mv.ScaledRMS  = Clamp(mv.ScaledRMS  - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

            Rect.top -= BarGap + BarHeight + 1.f;
        }

        ResetTransform(renderTarget);

        // Draw the channel names.
        if ((_XTextStyle->_ColorSource != ColorSource::None) && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.top = _GraphSettings->_FlipVertically ? _ClientRect.bottom : _ClientRect.top;

            const FLOAT dy = _GraphSettings->_FlipVertically ? -(_ClientSize.height / n) : (_ClientSize.height / n);

            for (const auto & mv : analysis._MeterValues)
            {
                Rect.bottom = Clamp(Rect.top + dy, 0.f, _ClientRect.bottom);

                if (_GraphSettings->_XAxisTop)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? _Bounds.left : _ClientRect.right + 1.f;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _ClientRect.left - 1.f : _Bounds.right;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                {
                    D2D1_RECT_F DebugRect = { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom };

                    _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue)); renderTarget->DrawRectangle(Rect, _DebugBrush);
                }
                #endif
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? _ClientRect.right + 1.f : _Bounds.left;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _Bounds.right : _ClientRect.left - 1.f;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                {
                    D2D1_RECT_F DebugRect = { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom };

                    _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue)); renderTarget->DrawRectangle(Rect, _DebugBrush);
                }
                #endif
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

        for (auto & mv : analysis._MeterValues)
        {
            Rect.right = Clamp(Rect.left + BarWidth, 0.f, _ClientSize.width - 1.f);

            // Draw the background.
            if (_BackgroundStyle->_ColorSource != ColorSource::None)
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

            // Draw the foreground.
            if (_PeakStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledPeak, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.bottom = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0.f, _ClientSize.height);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            #endif
            }

            if (_RMSStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledRMS, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.bottom = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0.f, _ClientSize.height);

            #ifndef _DEBUG_RENDER
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            #else
            {
                D2D1_RECT_F DebugRect = { Rect.left, 0.f, Rect.right, _ClientSize.height };

                _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
            }
            #endif
            }

            // Animate the scaled peak and RMS values.
            mv.ScaledPeak = Clamp(mv.ScaledPeak - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            mv.ScaledRMS  = Clamp(mv.ScaledRMS  - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

            Rect.left = Rect.right + 1.f + BarGap;
        }

    #ifdef _DEBUG_RENDER
        {
            D2D1_RECT_F DebugRect = D2D1::RectF(0.f, 0.f, _ClientSize.width, _ClientSize.height);

            _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Pink)); renderTarget->DrawRectangle(DebugRect, _DebugBrush);
        }
    #endif

        ResetTransform(renderTarget);

        // Draw the channel names.
        if ((_XTextStyle->_ColorSource != ColorSource::None) && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.left = _GraphSettings->_FlipHorizontally ? _ClientRect.right : _ClientRect.left;

            const FLOAT dx = _GraphSettings->_FlipHorizontally ? -(_ClientSize.width / n) : (_ClientSize.width / n);

            for (const auto & mv : analysis._MeterValues)
            {
                Rect.right = Clamp(Rect.left + dx, 0.f, _ClientRect.right);

                if (_GraphSettings->_XAxisTop)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? _ClientRect.bottom + 1.f : _Bounds.top;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _Bounds.bottom : _ClientRect.top - 1.f;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                {
                    D2D1_RECT_F DebugRect = { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom };

                    _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue)); renderTarget->DrawRectangle(Rect, _DebugBrush);
                }
                #endif
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? _Bounds.top : _ClientRect.bottom + 1.f;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _ClientRect.top - 1.f: _Bounds.bottom;

                #ifndef _DEBUG_RENDER
                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                #else
                {
                    D2D1_RECT_F DebugRect = { Rect.left + 1.f, Rect.top + 1.f, Rect.right, Rect.bottom };

                    _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue)); renderTarget->DrawRectangle(Rect, _DebugBrush);
                }
                #endif
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
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterRMSLevel, renderTarget, _Size, L"", &_RMSStyle);

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

    if (_RMSStyle)
    {
        _RMSStyle->ReleaseDeviceSpecificResources();
        _RMSStyle = nullptr;
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

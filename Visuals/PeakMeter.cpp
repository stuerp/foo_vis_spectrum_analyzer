
/** $VER: PeakMeter.cpp (2024.04.02) P. Stuer - Represents a peak meter. **/

#include "PeakMeter.h"

#include "Support.h"

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
/// Renders this instance.
/// </summary>
void PeakMeter::Render(ID2D1RenderTarget * renderTarget, Analysis & analysis)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

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
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectL, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    
                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectR, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                OldRect = Iter.RectL;
            }
        }
    }
    else
    {
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
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectL, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextStyle->_TextFormat, Iter.RectR, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                OldRect = Iter.RectL;
            }
        }
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
    const FLOAT TickSize = 2.f;
    const FLOAT BarGap = 1.f;

    if (_State->_HorizontalPeakMeter)
    {
        SetTransform(renderTarget, _Bounds);

        const FLOAT AvailableHeight = _YMax - _YMin;
        const FLOAT BarHeight = ::floor((AvailableHeight - (TickSize * 2.f)) / n);
        const FLOAT TotalHeight = (BarHeight * n) + BarGap;
        const FLOAT Offset = (AvailableHeight - TotalHeight) / 2.f;

        D2D1_RECT_F Rect = { _XMin, _YMax - (Offset + BarGap), _XMax, 0.f };

        for (auto & mv : analysis._MeterValues)
        {
            Rect.bottom = Rect.top - (BarHeight - BarGap);

            // Draw the background.
            if (_BackgroundStyle->_ColorSource != ColorSource::None)
            {
                Rect.right = _XMax;

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            // Draw the foreground.
            if (_PeakStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledPeak, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.right = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _XMin, _XMax);

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            if (_RMSStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledRMS, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.right = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _XMin, _XMax);

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            // Animate the scaled peak and RMS values.
            mv.ScaledPeak = Clamp(mv.ScaledPeak - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            mv.ScaledRMS  = Clamp(mv.ScaledRMS  - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

            Rect.top -= BarHeight;
        }

        ResetTransform(renderTarget);

//  renderTarget->DrawRectangle({ 0.f,   0.f, _Size.width,        _YMin }, _XTextStyle->_Brush);
//  renderTarget->DrawRectangle({ 0.f, _YMax, _Size.width, _Size.height }, _XTextStyle->_Brush);

        // Draw the channel names.
        if ((_XTextStyle->_ColorSource != ColorSource::None) && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.top = _GraphSettings->_FlipVertically ? _YMax - (Offset + BarGap) : _YMin + (Offset + BarGap);

            const FLOAT dy = _GraphSettings->_FlipVertically ? -BarHeight : BarHeight;

            for (const auto & mv : analysis._MeterValues)
            {
                Rect.bottom = Rect.top + (BarHeight - BarGap);

                if (_GraphSettings->_XAxisTop)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? _XMax         : 0.f;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _Bounds.right : _XMin;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
//                  renderTarget->FillRectangle(Rect, _XTextStyle->_Brush);
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? 0.f   : _XMax;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _XMin : _Bounds.right;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
//                  renderTarget->FillRectangle(Rect, _XTextStyle->_Brush);
                }

                Rect.top += dy;
            }
        }
    }
    else
    {
        SetTransform(renderTarget, _Bounds);

        const FLOAT AvailableWidth = _XMax - _XMin;
        const FLOAT BarWidth = ::floor((AvailableWidth - (TickSize * 2.f)) / n);
        const FLOAT TotalWidth = (BarWidth * n) + BarGap;
        const FLOAT Offset = (AvailableWidth - TotalWidth) / 2.f;

        D2D1_RECT_F Rect = { _XMin + Offset + BarGap, _YMin, 0.f, 0.f };

        for (auto & mv : analysis._MeterValues)
        {
            Rect.right = Rect.left + (BarWidth - BarGap);

            // Draw the background.
            if (_BackgroundStyle->_ColorSource != ColorSource::None)
            {
                Rect.bottom = _YMax;

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _BackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _BackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            // Draw the foreground.
            if (_PeakStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledPeak, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.bottom = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _YMin, _YMax);

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PeakStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _PeakStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            if (_RMSStyle->_ColorSource != ColorSource::None)
            {
                const double Value = Clamp(mv.ScaledRMS, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

                Rect.bottom = Map(Value, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _YMin, _YMax);

                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _RMSStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _RMSStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }

            // Animate the scaled peak and RMS values.
            mv.ScaledPeak = Clamp(mv.ScaledPeak - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            mv.ScaledRMS  = Clamp(mv.ScaledRMS  - 1.0, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);

            Rect.left += BarWidth;
        }

        ResetTransform(renderTarget);

//  renderTarget->DrawRectangle({   0.f, 0.f,       _XMin, _Size.height }, _XTextStyle->_Brush);
//  renderTarget->DrawRectangle({ _XMax, 0.f, _Size.width, _Size.height }, _XTextStyle->_Brush);

        // Draw the channel names.
        if ((_XTextStyle->_ColorSource != ColorSource::None) && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.left = _GraphSettings->_FlipHorizontally ? _XMax - Offset - BarGap : _XMin + Offset + BarGap;

            const FLOAT dx = _GraphSettings->_FlipHorizontally ? -BarWidth : BarWidth;

            for (const auto & mv : analysis._MeterValues)
            {
                Rect.right = Rect.left + (BarWidth - BarGap);

                if (_GraphSettings->_XAxisTop)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? _YMax          : 0.f;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _Bounds.bottom : _YMin;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
//                  renderTarget->FillRectangle(Rect, _XTextStyle->_Brush);
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? 0.f   : _YMax;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _YMin : _Bounds.bottom;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextStyle->_TextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
//                  renderTarget->FillRectangle(Rect, _XTextStyle->_Brush);
                }

                Rect.left += dx;
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

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void PeakMeter::Resize() noexcept
{
    if (!_IsResized)
        return;

    _YTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

    if (_State->_HorizontalPeakMeter)
    {
        _XMin = _Bounds.left   + (_GraphSettings->_XAxisBottom ? _XTextStyle->_TextWidth : 0);
        _XMax = _Bounds.right  - (_GraphSettings->_XAxisTop    ? _XTextStyle->_TextWidth : 0);

        if (_GraphSettings->_FlipVertically)
        {
            _YMin = _Bounds.top    + (_GraphSettings->_YAxisRight ? _YTextStyle->_TextHeight : 0.f);
            _YMax = _Bounds.bottom - (_GraphSettings->_YAxisLeft  ? _YTextStyle->_TextHeight : 0.f);
        }
        else
        {
            _YMin = _Bounds.top    + (_GraphSettings->_YAxisLeft  ? _YTextStyle->_TextHeight : 0.f);
            _YMax = _Bounds.bottom - (_GraphSettings->_YAxisRight ? _YTextStyle->_TextHeight : 0.f);
        }
    }
    else
    {
        _XMin = _Bounds.left   + (_GraphSettings->_YAxisLeft  ? _YTextStyle->_TextWidth : 0);
        _XMax = _Bounds.right  - (_GraphSettings->_YAxisRight ? _YTextStyle->_TextWidth : 0);

        if (_GraphSettings->_FlipVertically)
        {
            _YMin = _Bounds.top    + (_GraphSettings->_XAxisBottom ? _XTextStyle->_TextHeight : 0.f);
            _YMax = _Bounds.bottom - (_GraphSettings->_XAxisTop    ? _XTextStyle->_TextHeight : 0.f);
        }
        else
        {
            _YMin = _Bounds.top    + (_GraphSettings->_XAxisTop    ? _XTextStyle->_TextHeight : 0.f);
            _YMax = _Bounds.bottom - (_GraphSettings->_XAxisBottom ? _XTextStyle->_TextHeight : 0.f);
        }
    }

    if (_State->_HorizontalPeakMeter)
    {
        // Calculate the position of the labels based on the width.
        for (Label & Iter : _Labels)
        {
            FLOAT x = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipHorizontally ? _XMin : _XMax, !_GraphSettings->_FlipHorizontally ? _XMax : _XMin);

            // Don't generate any labels outside the bounds.
            if (!InRange(x, _XMin, _XMax))
                continue;

            Iter.PointL = D2D1_POINT_2F(x, _YMin);
            Iter.PointR = D2D1_POINT_2F(x, _YMax);

            x -= _YTextStyle->_TextWidth / 2.f; // Center the label horizontally on the tick.

            x = Clamp(x, _XMin - (_YTextStyle->_TextWidth / 2.f), _XMax + (_YTextStyle->_TextWidth / 2.f));

            Iter.RectL = { x, _Bounds.top, x + _YTextStyle->_TextWidth, _YMin };
            Iter.RectR = { x, _YMax,       x + _YTextStyle->_TextWidth, _Bounds.bottom };
        }
    }
    else
    {
        // Calculate the position of the labels based on the height.
        for (Label & Iter : _Labels)
        {
            FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipVertically ? _YMax : _YMin, !_GraphSettings->_FlipVertically ? _YMin : _YMax);

            // Don't generate any labels outside the bounds.
            if (!InRange(y, _YMin, _YMax))
                continue;

            Iter.PointL = D2D1_POINT_2F(_XMin, y);
            Iter.PointR = D2D1_POINT_2F(_XMax, y);

            y -= (_YTextStyle->_TextHeight / 2.f); // Center the label vertically on the tick.

            y = Clamp(y, _YMin - (_YTextStyle->_TextHeight / 2.f), _YMax + (_YTextStyle->_TextHeight / 2.f));

            Iter.RectL = { _Bounds.left + 2.f, y, _XMin         - 2.f, y + _YTextStyle->_TextHeight };
            Iter.RectR = { _XMax        + 2.f, y, _Bounds.right - 2.f, y + _YTextStyle->_TextHeight };
        }
    }

    _IsResized = false;
}

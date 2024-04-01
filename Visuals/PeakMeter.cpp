
/** $VER: PeakMeter.cpp (2024.04.01) P. Stuer - Represents a peak meter. **/

#include "PeakMeter.h"

#include "Support.h"

#include "DirectWrite.h"

#pragma hdrstop

PeakMeter::PeakMeter()
{
    _Bounds = { };
    _Size = { };

    _FontFamilyName = L"Segoe UI";
    _FontSize = 6.f;

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

    CreateDeviceIndependentResources();

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

    if (_State->_HorizontalPeakMeter)
    {
        _XMin = _Bounds.left   + (_GraphSettings->_XAxisBottom ? _XTextWidth : 0);
        _XMax = _Bounds.right  - (_GraphSettings->_XAxisTop    ? _XTextWidth : 0);

        _YMin = _Bounds.top    + (_GraphSettings->_FlipVertically ? (_GraphSettings->_YAxisRight ? _YTextHeight : 0.f) : (_GraphSettings->_YAxisLeft  ? _YTextHeight : 0.f));
        _YMax = _Bounds.bottom - (_GraphSettings->_FlipVertically ? (_GraphSettings->_YAxisLeft  ? _YTextHeight : 0.f) : (_GraphSettings->_YAxisRight ? _YTextHeight : 0.f));
    }
    else
    {
        _XMin = _Bounds.left   + (_GraphSettings->_YAxisLeft  ? _YTextWidth : 0);
        _XMax = _Bounds.right  - (_GraphSettings->_YAxisRight ? _YTextWidth : 0);

        _YMin = _Bounds.top    + (_GraphSettings->_FlipVertically ? (_GraphSettings->_XAxisBottom ? _XTextHeight : 0.f) : (_GraphSettings->_XAxisTop    ? _XTextHeight : 0.f));
        _YMax = _Bounds.bottom - (_GraphSettings->_FlipVertically ? (_GraphSettings->_XAxisTop    ? _XTextHeight : 0.f) : (_GraphSettings->_XAxisBottom ? _XTextHeight : 0.f));
    }

    // Calculate the position of the labels based on the height.
    for (Label & Iter : _Labels)
    {
        if (_State->_HorizontalPeakMeter)
        {
            FLOAT x = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipHorizontally ? _XMin : _XMax, !_GraphSettings->_FlipHorizontally ? _XMax : _XMin);

            // Don't generate any labels outside the bounds.
            if (!InRange(x, _XMin, _XMax))
                continue;

            Iter.PointL = D2D1_POINT_2F(x, _YMin);
            Iter.PointR = D2D1_POINT_2F(x, _YMax);

            x -= _YTextWidth / 2.f;

            x = Clamp(x, _XMin - (_YTextWidth / 2.f), _XMax + (_YTextWidth / 2.f));

            Iter.RectL = { x, _Bounds.top, x + _YTextWidth, _YMin };
            Iter.RectR = { x, _YMax,       x + _YTextWidth, _Bounds.bottom };
        }
        else
        {
            FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_GraphSettings->_FlipVertically ? _YMax : _YMin, !_GraphSettings->_FlipVertically ? _YMin : _YMax);

            // Don't generate any labels outside the bounds.
            if (!InRange(y, _YMin, _YMax))
                continue;

            Iter.PointL = D2D1_POINT_2F(_XMin, y);
            Iter.PointR = D2D1_POINT_2F(_XMax, y);

            y -= (_YTextHeight / 2.f);

            y = Clamp(y, _YMin - (_YTextHeight / 2.f), _YMax + (_YTextHeight / 2.f));

            Iter.RectL = { _Bounds.left, y, _XMin - 2.f,   y + _YTextHeight };
            Iter.RectR = { _XMax + 2.f,  y, _Bounds.right, y + _YTextHeight };
        }
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakMeter::Reset()
{
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
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextFormat, Iter.RectL, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextFormat, Iter.RectR, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

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
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextFormat, Iter.RectL, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _YTextFormat, Iter.RectR, _YTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

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

    if (_State->_HorizontalPeakMeter)
    {
        SetTransform(renderTarget, _Bounds);

        const FLOAT BarHeight = ::floor((_YMax - _YMin) / (FLOAT) analysis._MeterValues.size());

        D2D1_RECT_F Rect = { _XMin, _YMax, 0.f, _YMax - (BarHeight - 1.f) };

        for (auto & mv : analysis._MeterValues)
        {
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

            Rect.top    -= BarHeight;
            Rect.bottom -= BarHeight;
        }

        ResetTransform(renderTarget);

        // Draw the name of the values.
        if ((_XTextStyle->_ColorSource != ColorSource::None) && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.top    = _GraphSettings->_FlipVertically ? _YMax - BarHeight : _YMin;
            Rect.bottom = Rect.top + BarHeight;

            const FLOAT dy = _GraphSettings->_FlipVertically ? -BarHeight : BarHeight;

            for (const auto & mv : analysis._MeterValues)
            {
                if (_GraphSettings->_XAxisTop)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? _XMax         : 0.f;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _Bounds.right : _XMin;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.left  = _GraphSettings->_FlipHorizontally ? 0.f   : _XMax;
                    Rect.right = _GraphSettings->_FlipHorizontally ? _XMin : _Bounds.right;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                Rect.top    += dy;
                Rect.bottom += dy;
            }
        }
    }
    else
    {
        SetTransform(renderTarget, _Bounds);

        const FLOAT BarWidth = ::floor((_XMax - _XMin) / (FLOAT) analysis._MeterValues.size());

        D2D1_RECT_F Rect = { _XMin, _YMin, _XMin + (BarWidth - 1.f), 0.f };

        for (auto & mv : analysis._MeterValues)
        {
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

            Rect.left  += BarWidth;
            Rect.right += BarWidth;
        }

        ResetTransform(renderTarget);

        // Draw the name of the values.
        if ((_XTextStyle->_ColorSource != ColorSource::None) && (_GraphSettings->_XAxisTop || _GraphSettings->_XAxisBottom))
        {
            Rect.left = _GraphSettings->_FlipHorizontally ? _XMax - BarWidth : _XMin;
            Rect.right = Rect.left + BarWidth;

            const FLOAT dx = _GraphSettings->_FlipHorizontally ? -BarWidth : BarWidth;

            for (const auto & mv : analysis._MeterValues)
            {
                if (_GraphSettings->_XAxisTop)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? _YMax          : 0.f;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _Bounds.bottom : _YMin;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                if (_GraphSettings->_XAxisBottom)
                {
                    Rect.top    = _GraphSettings->_FlipVertically ? 0.f   : _YMax;
                    Rect.bottom = _GraphSettings->_FlipVertically ? _YMin : _Bounds.bottom;

                    renderTarget->DrawText(mv.Name.c_str(), (UINT) mv.Name.size(), _XTextFormat, Rect, _XTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }

                Rect.left  += dx;
                Rect.right += dx;
            }
        }
    }

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(OldAntialiasMode);
}

/// <summary>
/// Creates resources which are not bound to any D3D device.
/// </summary>
HRESULT PeakMeter::CreateDeviceIndependentResources() noexcept
{
    if ((_XTextFormat != nullptr) && (_YTextFormat != nullptr))
        return S_OK;

    const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

    HRESULT hr = _DirectWrite.CreateTextFormat(_FontFamilyName, FontSize, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, _XTextFormat);

    if (SUCCEEDED(hr))
    {
        auto Text = std::wstring(L"WWW");

        hr = _DirectWrite.GetTextMetrics(_XTextFormat, Text, _XTextWidth, _XTextHeight);
    }

    if (SUCCEEDED(hr))
    {
        const DWRITE_TEXT_ALIGNMENT ta = _State->_HorizontalPeakMeter ? DWRITE_TEXT_ALIGNMENT_CENTER : (!_GraphSettings->_FlipHorizontally ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);

        hr = _DirectWrite.CreateTextFormat(_FontFamilyName, FontSize, ta, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, _YTextFormat);

        if (SUCCEEDED(hr))
        {
            auto Text = std::wstring(L"-999");

            hr = _DirectWrite.GetTextMetrics(_YTextFormat, Text, _YTextWidth, _YTextHeight);
        }
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void PeakMeter::ReleaseDeviceIndependentResources() noexcept
{
    _YTextFormat.Release();
    _XTextFormat.Release();
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
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterBackground, renderTarget, _Size, &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterPeakLevel, renderTarget, _Size, &_PeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::PeakMeterRMSLevel, renderTarget, _Size, &_RMSStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, _Size, &_XTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, _Size, &_YTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, _Size, &_YLineStyle);

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

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.f, 0.f, 0.f, 1.f)), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

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

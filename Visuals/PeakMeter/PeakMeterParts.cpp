
/** $VER: PeakMeterParts.cpp (2025.11.09) P. Stuer - Implements the parts of a peak meter. **/

#include "pch.h"

#include "PeakMeterParts.h"

#include "Support.h"
#include "Log.h"

#include "libmsc.h"

#pragma hdrstop

/// <summary>
/// Initializes the boundaries of this instance and prepares the transform.
/// </summary>
void bar_t::SetRect(const D2D1_RECT_F & rect) noexcept
{
    __super::SetRect(rect);

    // Precalculate the coordinates of all the parts.
    if (_State->_HorizontalPeakMeter)
    {
        if (_Settings->_FlipHorizontally)
        {
            FLOAT x = _Rect.right;

            if (_NameStyle->IsEnabled() && _Settings->_XAxisTop)
            {
                _TopNameRect = { _Rect.left, _Rect.top, _Rect.left + _NameStyle->_Width, _Rect.bottom };
                _Rect.right -= _NameStyle->_Width;
            }

            if (_RMSTextStyle->IsEnabled())
            {
                _RMSRect = { x - _RMSTextStyle->_Width, _Rect.top, x, _Rect.bottom };
                x = _RMSRect.left;
                _Rect.left += _RMSTextStyle->_Width;
            }

            if (_PeakTextStyle->IsEnabled())
            {
                _PeakRect = { x - _PeakTextStyle->_Width, _Rect.top, x, _Rect.bottom };
                x = _PeakRect.left;
                _Rect.left += _PeakTextStyle->_Width;
            }

            if (_NameStyle->IsEnabled() && _Settings->_XAxisBottom)
            {
                _BottomNameRect = { x - _NameStyle->_Width, _Rect.top, x, _Rect.bottom };
                x = _BottomNameRect.left;
                _Rect.left += _NameStyle->_Width;
            }
        }
        else
        {
            if (_RMSTextStyle->IsEnabled())
            {
                _RMSRect = { _Rect.left, _Rect.top, _Rect.left + _RMSTextStyle->_Width, _Rect.bottom };
                _Rect.left = _RMSRect.right;
            }

            if (_PeakTextStyle->IsEnabled())
            {
                _PeakRect = { _Rect.left, _Rect.top, _Rect.left + _PeakTextStyle->_Width, _Rect.bottom};
                _Rect.left = _PeakRect.right;
            }

            if (_NameStyle->IsEnabled())
            {
                if (_Settings->_XAxisBottom)
                {
                    _BottomNameRect = { _Rect.left, _Rect.top, _Rect.left + _NameStyle->_Width, _Rect.bottom };
                    _Rect.left = _BottomNameRect.right;
                }

                if (_Settings->_XAxisTop)
                {
                    _TopNameRect = { _Rect.right - _NameStyle->_Width, _Rect.top, _Rect.right, _Rect.bottom };
                    _Rect.right = _TopNameRect.left;
                }
            }
        }
    }
    else
    {
        if (_Settings->_FlipVertically)
        {
            if (_RMSTextStyle->IsEnabled())
            {
                _RMSRect = { _Rect.left, _Rect.top, _Rect.right, _Rect.top + _RMSTextStyle->_Height };
                _Rect.top = _RMSRect.bottom;
            }

            if (_PeakTextStyle->IsEnabled())
            {
                _PeakRect = { _Rect.left, _Rect.top, _Rect.right, _Rect.top + _PeakTextStyle->_Height };
                _Rect.top = _PeakRect.bottom;
            }

            if (_NameStyle->IsEnabled())
            {
                if (_Settings->_XAxisBottom)
                {
                    _BottomNameRect = { _Rect.left, _Rect.top, _Rect.right, _Rect.top + _NameStyle->_Height };
                    _Rect.top = _BottomNameRect.bottom;
                }

                if (_Settings->_XAxisTop)
                {
                    _TopNameRect = { _Rect.left, _Rect.bottom - _NameStyle->_Height, _Rect.right, _Rect.bottom };
                    _Rect.bottom = _TopNameRect.top;
                }
            }
        }
        else
        {
            FLOAT y = _Rect.bottom;

            if (_NameStyle->IsEnabled() && _Settings->_XAxisTop)
            {
                _TopNameRect = { _Rect.left, _Rect.top, _Rect.right, _Rect.top + _NameStyle->_Height };
                _Rect.bottom -= _NameStyle->_Height;
            }

            if (_RMSTextStyle->IsEnabled())
            {
                _RMSRect = { _Rect.left, y - _RMSTextStyle->_Height, _Rect.right, y };
                y = _RMSRect.top;
                _Rect.top += _RMSTextStyle->_Height;
            }

            if (_PeakTextStyle->IsEnabled())
            {
                _PeakRect = { _Rect.left, y - _PeakTextStyle->_Height, _Rect.right, y };
                y = _PeakRect.top;
                _Rect.top += _PeakTextStyle->_Height;
            }

            if (_NameStyle->IsEnabled() && _Settings->_XAxisBottom)
            {
                _BottomNameRect = { _Rect.left, y - _NameStyle->_Height, _Rect.right, y };
                y = _BottomNameRect.top;
                _Rect.top += _NameStyle->_Height;
            }
        }
    }

    _Size = { _Rect.right - _Rect.left, _Rect.bottom - _Rect.top };

    // Prealculate the transform.
    _Transform = D2D1::Matrix3x2F::Identity();

    // Compute center of the area.
    const D2D1_POINT_2F Center = { (rect.left + rect.right) / 2.f, (rect.top + rect.bottom) / 2.f };

    if (_Settings->_FlipHorizontally)
        _Transform = D2D1::Matrix3x2F::Scale(-1.0f, 1.0f, Center);

    if (_State->_HorizontalPeakMeter)
    {
        if (_Settings->_FlipVertically)
            _Transform = _Transform * D2D1::Matrix3x2F::Scale(1.0f, -1.0f, Center);
    }
    else
    {
        if (!_Settings->_FlipVertically)
            _Transform = _Transform * D2D1::Matrix3x2F::Scale(1.0f, -1.0f, Center);
    }

    _LEDSize = _State->_LEDSize + _State->_LEDGap;
}
/*
/// <summary>
/// Binds the Direct2D resources to this instance and adjusts the layout using the text metrics.
/// </summary>
void bar_t::Bind(ID2D1DeviceContext * deviceContext, style_t * backgroundStyle, style_t * peakStyle, style_t * peak0dBStyle, style_t * maxPeakStyle, style_t * peakTextStyle, style_t * rmsStyle, style_t * rms0dBStyle, style_t * rmsTextStyle, style_t * nameStyle, style_t * scaleTextStyle, style_t * scaleLineStyle, ID2D1SolidColorBrush * debugBrush, ID2D1Bitmap * opacityMask) noexcept
{
    __super::Bind
    (
        deviceContext,
        backgroundStyle,
        peakStyle,
        peak0dBStyle,
        maxPeakStyle,
        peakTextStyle,
        rmsStyle,
        rms0dBStyle,
        rmsTextStyle,
        nameStyle,
        scaleTextStyle,
        scaleLineStyle,
        debugBrush,
        opacityMask
    );
}
*/
/// <summary>
/// Renders this instance.
/// </summary>
void bar_t::Render() const noexcept
{
    D2D1_RECT_F Rect = _Rect;

    _DeviceContext->SetTransform(_Transform);

    if (_State->_HorizontalPeakMeter)
    {
        // Draw the background.
        if (_BackgroundStyle->IsEnabled())
        {
            DrawHorizontalRectangle(Rect, _BackgroundStyle);
        }

        // Draw the foreground (Peak).
        {
            Rect.right = _Rect.left + (FLOAT) _Measurement->PeakNormalized * _Size.width;

            DrawHorizontalRectangle(Rect, _PeakStyle);

            // Draw the foreground (Peak, Measurement > 0dBFS).
            if (_Peak0dBStyle->IsEnabled() && (_Measurement->PeakNormalized > _dBFSZeroNormalized))
            {
                Rect.left  = _Rect.left + (FLOAT) _dBFSZeroNormalized * _Size.width;
                Rect.right = _Rect.left + (FLOAT) _Measurement->PeakNormalized * _Size.width;

                DrawHorizontalRectangle(Rect, _Peak0dBStyle);
            }
        }
        // Draw the foreground (Peak Top).
        if ((_State->_PeakMode != PeakMode::None) && (_Measurement->MaxPeakNormalized > 0.) && _MaxPeakStyle->IsEnabled())
        {
            const FLOAT x = (FLOAT) _Measurement->MaxPeakNormalized * _Size.width;

            Rect.left  = _Rect.left + x;
            Rect.right = _Rect.left + x;

            if (!_State->_LEDMode)
            {
                Rect.left  -= _MaxPeakStyle->_Thickness / 2.f;
                Rect.right += _MaxPeakStyle->_Thickness / 2.f;
            }

            const FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) _Measurement->Opacity : _MaxPeakStyle->_Opacity;

            _MaxPeakStyle->_Brush->SetOpacity(Opacity);

            DrawHorizontalRectangle(Rect, _MaxPeakStyle);
        }

        // Draw the foreground (RMS).
        if (_RMSStyle->IsEnabled())
        {
            Rect.left  = _Rect.left;
            Rect.right = _Rect.left + (FLOAT) _Measurement->RMSNormalized * _Size.width;

            DrawHorizontalRectangle(Rect, _RMSStyle);

            // Draw the foreground (RMS, Measurement > 0dBFS).
            if (_RMS0dBStyle->IsEnabled() && (_Measurement->RMSNormalized > _dBFSZeroNormalized))
            {
                Rect.left  = _Rect.left + (FLOAT) _dBFSZeroNormalized * _Size.width;
                Rect.right = _Rect.left + (FLOAT) _Measurement->RMSNormalized * _Size.width;

                DrawHorizontalRectangle(Rect, _RMS0dBStyle);
            }
        }
    }
    else
    {
        // Draw the background.
        if (_BackgroundStyle->IsEnabled())
        {
            DrawVerticalRectangle(Rect, _BackgroundStyle);
        }

        // Draw the foreground (Peak).
        if (_PeakStyle->IsEnabled())
        {
            Rect.bottom = _Rect.top + ((FLOAT) _Measurement->PeakNormalized * _Size.height);

            DrawVerticalRectangle(Rect, _PeakStyle);

            // Draw the foreground (Peak, Measurement > 0dBFS).
            if (_Peak0dBStyle->IsEnabled() && (_Measurement->PeakNormalized > _dBFSZeroNormalized))
            {
                Rect.top    = _Rect.top + (FLOAT) _dBFSZeroNormalized * _Size.height;
                Rect.bottom = _Rect.top + (FLOAT) _Measurement->PeakNormalized * _Size.height;

                DrawVerticalRectangle(Rect, _Peak0dBStyle);
            }
        }

        // Draw the foreground (Peak Top).
        if ((_State->_PeakMode != PeakMode::None) && (_Measurement->MaxPeakNormalized > 0.) && _MaxPeakStyle->IsEnabled())
        {
            const FLOAT y = (FLOAT) _Measurement->MaxPeakNormalized * _Size.height;

            Rect.top    = _Rect.top + y;
            Rect.bottom = _Rect.top + y;

            if (!_State->_LEDMode)
            {
                Rect.top    -= _MaxPeakStyle->_Thickness / 2.f;
                Rect.bottom += _MaxPeakStyle->_Thickness / 2.f;
            }

            const FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) _Measurement->Opacity : _MaxPeakStyle->_Opacity;

            _MaxPeakStyle->_Brush->SetOpacity(Opacity);

            DrawVerticalRectangle(Rect, _MaxPeakStyle);
        }

        // Draw the foreground (RMS).
        if (_RMSStyle->IsEnabled())
        {
            Rect.top    = _Rect.top;
            Rect.bottom = _Rect.top + (FLOAT) _Measurement->RMSNormalized * _Size.height;

            DrawVerticalRectangle(Rect, _RMSStyle);

            // Draw the foreground (Peak, Measurement > 0dBFS).
            if (_RMS0dBStyle->IsEnabled() && (_Measurement->RMSNormalized > _dBFSZeroNormalized))
            {
                Rect.top    = _Rect.top + (FLOAT) _dBFSZeroNormalized * _Size.height;
                Rect.bottom = _Rect.top + (FLOAT) _Measurement->RMSNormalized * _Size.height;

                DrawVerticalRectangle(Rect, _RMS0dBStyle);
            }
        }
    }

    _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

    // Draw the text.
    {
        WCHAR Text[16];

        // Draw the channel name.
        if (_NameStyle->IsEnabled())
        {
            if (_Settings->_XAxisTop)
            {
//              _DeviceContext->DrawRectangle(_TopNameRect, _DebugBrush);
                _DeviceContext->DrawText(_Measurement->Name.c_str(), (UINT) _Measurement->Name.size(), _NameStyle->_TextFormat, _TopNameRect, _NameStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_Settings->_XAxisBottom)
            {
//              _DeviceContext->DrawRectangle(_BottomNameRect, _DebugBrush);
                _DeviceContext->DrawText(_Measurement->Name.c_str(), (UINT) _Measurement->Name.size(), _NameStyle->_TextFormat, _BottomNameRect, _NameStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        // Draw the peak readout.
        if (_PeakTextStyle->IsEnabled())
        {
            if (::isfinite(_Measurement->Peak))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", _Measurement->Peak);
            else
                ::wcscpy_s(Text, _countof(Text), NegativeInfinity);

            _DeviceContext->DrawText(Text, (UINT) ::wcslen(Text), _PeakTextStyle->_TextFormat, _PeakRect, _PeakTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        // Draw the RMS readout.
        if (_RMSTextStyle->IsEnabled())
        {
            if (::isfinite(_Measurement->RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", _Measurement->RMS);
            else
                ::wcscpy_s(Text, _countof(Text), NegativeInfinity);

            _DeviceContext->DrawText(Text, (UINT) ::wcslen(Text), _RMSTextStyle->_TextFormat, _RMSRect, _RMSTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }
}

/// <summary>
/// Draws a horizontal rectangle using the specified style.
/// </summary>
void bar_t::DrawHorizontalRectangle(D2D1_RECT_F & rect, const style_t * style) const noexcept
{
//  _DeviceContext->DrawRectangle(rect, _DebugBrush);

    if (_State->_LEDMode)
    {
        if (_State->_LEDIntegralSize)
        {
            rect.left  = std::max(_Rect.left,  _Rect.left + std::floorf((rect.left  - _Rect.left) / _LEDSize) * _LEDSize);
            rect.right = std::min(_Rect.right, _Rect.left + std::ceilf ((rect.right - _Rect.left) / _LEDSize) * _LEDSize);
        }

        const D2D1_RECT_F Src = { 0.f, _Rect.top, rect.right - rect.left, _Rect.bottom };

        _DeviceContext->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);

        _DeviceContext->FillOpacityMask(_OpacityMask, style->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, rect, Src);

        _DeviceContext->PopAxisAlignedClip();
    }
    else
        _DeviceContext->FillRectangle(rect, style->_Brush);
}

/// <summary>
/// Draws a vertical rectangle using the specified style.
/// </summary>
void bar_t::DrawVerticalRectangle(D2D1_RECT_F & rect, const style_t * style) const noexcept
{
//  _DeviceContext->DrawRectangle(rect, _DebugBrush);

    if (_State->_LEDMode)
    {
        if (_State->_LEDIntegralSize)
        {
            rect.top    = std::max(_Rect.top,    _Rect.top + std::floorf((rect.top    - _Rect.top) / _LEDSize) * _LEDSize);
            rect.bottom = std::min(_Rect.bottom, _Rect.top + std::ceilf ((rect.bottom - _Rect.top) / _LEDSize) * _LEDSize);

        }

        const D2D1_RECT_F Src = { _Rect.left, 0.f, _Rect.right, rect.bottom - rect.top };

        _DeviceContext->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);

        _DeviceContext->FillOpacityMask(_OpacityMask, style->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, rect, Src);

        _DeviceContext->PopAxisAlignedClip();
    }
    else
        _DeviceContext->FillRectangle(rect, style->_Brush);
}

/// <summary>
/// Initializes the boundaries of this instance and prepares the transform.
/// </summary>
void scale_t::SetRect(const D2D1_RECT_F & rect) noexcept
{
    __super::SetRect(rect);

    _Labels.clear();

    if (_Settings->_YAxisMode == YAxisMode::None)
        return;

    // Precalculate the coordinates of all the parts.
    if (_State->_HorizontalPeakMeter)
    {
        if (_Settings->_FlipHorizontally)
        {
            if (_NameStyle->IsEnabled() && _Settings->_XAxisTop)
                _Rect.left += _NameStyle->_Width;

            if (_RMSTextStyle->IsEnabled())
                _Rect.right -= _RMSTextStyle->_Width;

            if (_PeakTextStyle->IsEnabled())
                _Rect.right -= _PeakTextStyle->_Width;

            if (_NameStyle->IsEnabled() && _Settings->_XAxisBottom)
                _Rect.right -= _NameStyle->_Width;
        }
        else
        {
            if (_RMSTextStyle->IsEnabled())
                _Rect.left += _RMSTextStyle->_Width;

            if (_PeakTextStyle->IsEnabled())
                _Rect.left += _PeakTextStyle->_Width;

            if (_NameStyle->IsEnabled())
            {
                if (_Settings->_XAxisBottom)
                    _Rect.left += _NameStyle->_Width;

                if (_Settings->_XAxisTop)
                    _Rect.right -= _NameStyle->_Width;
            }
        }
    }
    else
    {
        if (_Settings->_FlipVertically)
        {
            if (_RMSTextStyle->IsEnabled())
                _Rect.top += _RMSTextStyle->_Height;

            if (_PeakTextStyle->IsEnabled())
                _Rect.top += _PeakTextStyle->_Height;

            if (_NameStyle->IsEnabled())
            {
                if (_Settings->_XAxisBottom)
                    _Rect.top += _NameStyle->_Height;

                if (_Settings->_XAxisTop)
                    _Rect.bottom -= _NameStyle->_Height;
            }
        }
        else
        {
            if (_NameStyle->IsEnabled() && _Settings->_XAxisBottom)
                _Rect.bottom -= _NameStyle->_Height;

            if (_RMSTextStyle->IsEnabled())
                _Rect.bottom -= _RMSTextStyle->_Height;

            if (_PeakTextStyle->IsEnabled())
                _Rect.bottom -= _PeakTextStyle->_Height;

            if (_NameStyle->IsEnabled() && _Settings->_XAxisTop)
                _Rect.top += _NameStyle->_Height;
        }

        _Size = { _Rect.right - _Rect.left, _Rect.bottom - _Rect.top };
    }

    // Create the labels.
    {
        const double Epsilon = 1.e-3;

        if (_State->_HorizontalPeakMeter)
        {
            const FLOAT xMin = (!_Settings->_FlipHorizontally ? _Rect.left  : _Rect.right);
            const FLOAT xMax = (!_Settings->_FlipHorizontally ? _Rect.right : _Rect.left);

            const FLOAT dw = _ScaleTextStyle->_Width / 2.f;

            for (double Amplitude = _Settings->_AmplitudeLo; Amplitude <= (_Settings->_AmplitudeHi + Epsilon); Amplitude -= _Settings->_AmplitudeStep)
            {
                WCHAR Text[16] = { };

                ::StringCchPrintfW(Text, _countof(Text), L"%+d", (int) Amplitude);

                FLOAT y1, y2;

                switch (_TextAlignment)
                {
                    case DWRITE_TEXT_ALIGNMENT_LEADING:
                        y1 = _Rect.top + 1.f;
                        y2 = y1 + _TickSize;
                        break;

                    case DWRITE_TEXT_ALIGNMENT_CENTER:
                        y1 = _Rect.top + 1.f;
                        y2 = _Rect.bottom - 1.f;
                        break;

                    case DWRITE_TEXT_ALIGNMENT_TRAILING:
                        y1 = _Rect.bottom - 1.f;
                        y2 = y1 - _TickSize;
                        break;

                    case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
                    default:
                        y1 = y2 = 0.f;
                }

                const FLOAT x = msc::Map(_Settings->ScaleAmplitude(ToMagnitude(Amplitude)), 0., 1., xMin, xMax);

                label_t Label =
                {
                    Text, Amplitude, false,
                    D2D1::Point2F(x, y1),
                    D2D1::Point2F(x, y2),
                    D2D1::RectF
                    (
                        x - dw,
                        (_TextAlignment == DWRITE_TEXT_ALIGNMENT_LEADING) ? y2 + 1.f : _Rect.top + 1.f,
                        x - dw + _ScaleTextStyle->_Height,
                        (_TextAlignment == DWRITE_TEXT_ALIGNMENT_LEADING) ? _Rect.bottom - 1.f : y2 - 1.f
                    )
                };

                _Labels.push_back(Label);
            }
        }
        else
        {
            const FLOAT yMin = (!_Settings->_FlipVertically ? _Rect.bottom : _Rect.top);
            const FLOAT yMax = (!_Settings->_FlipVertically ? _Rect.top : _Rect.bottom);

            const FLOAT dh = _ScaleTextStyle->_Height / 2.f;

            for (double Amplitude = _Settings->_AmplitudeLo; Amplitude <= (_Settings->_AmplitudeHi + Epsilon); Amplitude -= _Settings->_AmplitudeStep)
            {
                WCHAR Text[16] = { };

                ::StringCchPrintfW(Text, _countof(Text), L"%+d", (int) Amplitude);

                FLOAT x1, x2;

                switch (_TextAlignment)
                {
                    case DWRITE_TEXT_ALIGNMENT_LEADING:
                        x1 = _Rect.left + 1.f;
                        x2 = x1 + _TickSize;
                        break;

                    case DWRITE_TEXT_ALIGNMENT_CENTER:
                        x1 = _Rect.left  + 1.f;
                        x2 = _Rect.right - 1.f;
                        break;

                    case DWRITE_TEXT_ALIGNMENT_TRAILING:
                        x1 = _Rect.right - 1.f;
                        x2 = x1 - _TickSize;
                        break;

                    case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
                    default:
                        x1 = x2 = 0.f;
                }

                const FLOAT y = msc::Map(_Settings->ScaleAmplitude(ToMagnitude(Amplitude)), 0., 1., yMin, yMax);

                label_t Label =
                {
                    Text, Amplitude, false,
                    D2D1::Point2F(x1, y),
                    D2D1::Point2F(x2, y),
                    D2D1::RectF
                    (
                        (_TextAlignment == DWRITE_TEXT_ALIGNMENT_LEADING) ? x2 + 1.f : _Rect.left + 1.f,
                        y - dh,
                        (_TextAlignment == DWRITE_TEXT_ALIGNMENT_LEADING) ? _Rect.right - 1.f : x2 - 1.f,
                        y - dh + _ScaleTextStyle->_Height
                    )
                };

                _Labels.push_back(Label);
            }
        }
    }
}
/*
/// <summary>
/// Binds the Direct2D resources to this instance and adjusts the layout using the text metrics.
/// </summary>
void scale_t::Bind(ID2D1DeviceContext * deviceContext, style_t * backgroundStyle, style_t * peakStyle, style_t * peak0dBStyle, style_t * maxPeakStyle, style_t * peakTextStyle, style_t * rmsStyle, style_t * rms0dBStyle, style_t * rmsTextStyle, style_t * nameStyle, style_t * scaleTextStyle, style_t * scaleLineStyle, ID2D1SolidColorBrush * debugBrush, ID2D1Bitmap * opacityMask) noexcept
{
    __super::Bind
    (
        deviceContext,
        backgroundStyle,
        peakStyle,
        peak0dBStyle,
        maxPeakStyle,
        peakTextStyle,
        rmsStyle,
        rms0dBStyle,
        rmsTextStyle,
        nameStyle,
        scaleTextStyle,
        scaleLineStyle,
        debugBrush,
        opacityMask
    );
}
*/
/// <summary>
/// Renders this instance.
/// </summary>
void scale_t::Render() const noexcept
{
    _ScaleTextStyle->SetHorizontalAlignment(_TextAlignment);
    _ScaleTextStyle->SetVerticalAlignment(_ParagraphAlignment);

    for (const label_t & Label : _Labels)
    {
//      _DeviceContext->DrawRectangle(Label.Rect, _DebugBrush);
        _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _ScaleTextStyle->_TextFormat, Label.Rect, _ScaleTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

        if (!IsCenter())
            _DeviceContext->DrawLine(Label.P1, Label.P2, _ScaleLineStyle->_Brush, _ScaleLineStyle->_Thickness);
    }
}

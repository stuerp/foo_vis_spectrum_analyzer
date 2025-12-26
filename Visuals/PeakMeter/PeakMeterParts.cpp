
/** $VER: PeakMeterParts.cpp (2025.12.26) P. Stuer - Implements the parts of a peak meter. **/

#include "pch.h"

#include "PeakMeterParts.h"

#include "Support.h"
#include "Log.h"

#include "libmsc.h"

#pragma hdrstop

/// <summary>
/// Initializes the boundaries of this instance.
/// </summary>
void part_t::SetRect(const D2D1_RECT_F & rect) noexcept
{
    _Rect = rect;
    _Size = { rect.right - rect.left, rect.bottom - rect.top };

    // Precalculate the coordinates of all the parts.
    if (_State->_HorizontalPeakMeter)
    {
        FLOAT & x1 = _Rect.left;
        FLOAT & x2 = _Rect.right;

        if (_Settings->_FlipHorizontally)
        {
            FLOAT w = (_NameStyle->IsEnabled() && _Settings->_XAxisTop) ? _NameStyle->_Width : 0.f;

            _TopNameRect = { x1, _Rect.top, x1 + w, _Rect.bottom };
            x1 += w;

            w = _RMSTextStyle->IsEnabled() ? _RMSTextStyle->_Width : 0.f;

            _RMSRect = { x2 - w, _Rect.top, x2, _Rect.bottom };
            x2 -= w;

            w = _PeakTextStyle->IsEnabled() ? _PeakTextStyle->_Width : 0.f;

            _PeakRect = { x2 - w, _Rect.top, x2, _Rect.bottom };
            x2 -= w;

            w = (_NameStyle->IsEnabled() && _Settings->_XAxisBottom) ? _NameStyle->_Width : 0.f;

            _BottomNameRect = { x2 - w, _Rect.top, x2, _Rect.bottom };
            x2 -= w;
        }
        else
        {
            FLOAT w = _RMSTextStyle->IsEnabled() ? _RMSTextStyle->_Width : 0.f;

            _RMSRect = { x1, _Rect.top, x1 + w, _Rect.bottom };
            x1 += w;

            w = _PeakTextStyle->IsEnabled() ? _PeakTextStyle->_Width : 0.f;

            _PeakRect = { x1, _Rect.top, x1 + w, _Rect.bottom };
            x1 += w;

            if (_NameStyle->IsEnabled())
            {
                w = _Settings->_XAxisBottom ? _NameStyle->_Width : 0.f;

                _BottomNameRect = { x1, _Rect.top, x1 + w, _Rect.bottom };
                x1 += w;

                w = _Settings->_XAxisTop ? _NameStyle->_Width : 0.f;

                _TopNameRect = { x2 - w, _Rect.top, x2, _Rect.bottom };
                x2 -= w;
            }
        }
    }
    else
    {
        FLOAT & y1 = _Rect.top;
        FLOAT & y2 = _Rect.bottom;

        if (_Settings->_FlipVertically)
        {
            FLOAT h = _RMSTextStyle->IsEnabled() ? _RMSTextStyle->_Height : 0.f;

            _RMSRect = { _Rect.left, y1, _Rect.right, y1 + h };
            y1 += h;

            h = _PeakTextStyle->IsEnabled() ? _PeakTextStyle->_Height : 0.f;

            _PeakRect = { _Rect.left, y1, _Rect.right, y1 + h };
            y1 += h;

            if (_NameStyle->IsEnabled())
            {
                h = _Settings->_XAxisBottom ? _NameStyle->_Height : 0.f;

                _BottomNameRect = { _Rect.left, y1, _Rect.right, y1 + h };
                y1 += h;

                h = _Settings->_XAxisTop ? _NameStyle->_Height : 0.f;

                _TopNameRect = { _Rect.left, y2 - h, _Rect.right, y2 };
                y2 -= h;
            }
        }
        else
        {
            FLOAT h = (_NameStyle->IsEnabled() && _Settings->_XAxisTop) ? _NameStyle->_Height : 0.f;

            _TopNameRect = { _Rect.left, y1, _Rect.right, y1 + h };
            y1 += h;

            h = _RMSTextStyle->IsEnabled() ? _RMSTextStyle->_Height : 0.f;

            _RMSRect = { _Rect.left, y2 - h, _Rect.right, y2 };
            y2 -= h;

            h = _PeakTextStyle->IsEnabled() ? _PeakTextStyle->_Height : 0.f;

            _PeakRect = { _Rect.left, y2 - h, _Rect.right, y2 };
            y2 -= h;

            h = (_NameStyle->IsEnabled() && _Settings->_XAxisBottom) ? _NameStyle->_Height : 0.f;

            _BottomNameRect = { _Rect.left, y2 - h, _Rect.right, y2 };
            y2 -= h;
        }
    }

    _Size = { _Rect.right - _Rect.left, _Rect.bottom - _Rect.top };

    CreateAxis();
}

/// <summary>
/// Binds the Direct2D resources to this instance.
/// </summary>
void part_t::Bind(ID2D1DeviceContext * deviceContext, style_t * backgroundStyle, style_t * peakStyle, style_t * peak0dBStyle, style_t * maxPeakStyle, style_t * peakTextStyle, style_t * rmsStyle, style_t * rms0dBStyle, style_t * rmsTextStyle, style_t * nameStyle, style_t * scaleTextStyle, style_t * scaleLineStyle, ID2D1SolidColorBrush * debugBrush, ID2D1Bitmap * opacityMask) noexcept
{
    _DeviceContext = deviceContext;

    _BackgroundStyle = backgroundStyle;

    _PeakStyle = peakStyle;
    _Peak0dBStyle = peak0dBStyle;
    _MaxPeakStyle = maxPeakStyle;
    _PeakTextStyle = peakTextStyle;

    _RMSStyle = rmsStyle;
    _RMS0dBStyle = rms0dBStyle;
    _RMSTextStyle = rmsTextStyle;

    _NameStyle = nameStyle;

    _ScaleTextStyle = scaleTextStyle;
    _ScaleLineStyle = scaleLineStyle;

    _DebugBrush = debugBrush;

    _OpacityMask = opacityMask;
}

/// <summary>
/// Unbinds the Direct2D resources from this instance.
/// </summary>
void part_t::Unbind() noexcept
{
    _DeviceContext.Release();

    _BackgroundStyle = nullptr;

    _PeakStyle = nullptr;
    _Peak0dBStyle = nullptr;
    _MaxPeakStyle = nullptr;
    _PeakTextStyle = nullptr;

    _RMSStyle = nullptr;
    _RMS0dBStyle = nullptr;
    _RMSTextStyle = nullptr;

    _NameStyle = nullptr;

    _ScaleTextStyle = nullptr;
    _ScaleLineStyle = nullptr;

    _DebugBrush.Release();

    _OpacityMask.Release();
}

/// <summary>
/// Creates the labels and tick coordinates.
/// </summary>
void part_t::CreateAxis() noexcept
{
    _Labels.clear();

    if (_Settings->_YAxisMode == YAxisMode::None)
        return;

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
            D2D1_RECT_F r;

            switch (_ParagraphAlignment)
            {
                case DWRITE_PARAGRAPH_ALIGNMENT_NEAR:
                    y1 = _Rect.top;
                    y2 = y1 + _TickSize;
                    r = { 0.f, y2, 0.f, _Rect.bottom };
                    break;

                case DWRITE_PARAGRAPH_ALIGNMENT_CENTER:
                    y1 = _Rect.top + 1.f;
                    y2 = _Rect.bottom - 1.f;
                    r = { 0.f, _Rect.top, 0.f, _Rect.bottom };
                    break;

                case DWRITE_PARAGRAPH_ALIGNMENT_FAR:
                    y2 = _Rect.bottom;
                    y1 = y2 - _TickSize;
                    r = { 0.f, _Rect.top, 0.f, y1 };
                    break;

                default:
                    y1 = y2 = 0.f;
            }

            const FLOAT x = msc::Map(_Settings->ScaleAmplitude(ToMagnitude(Amplitude)), 0., 1., xMin, xMax);

            r.left  = x - dw;
            r.right = r.left + _ScaleTextStyle->_Width;

            label_t Label =
            {
                Text, Amplitude, false,
                D2D1::Point2F(x, y1),
                D2D1::Point2F(x, y2),
                r
            };

            _Labels.push_back(Label);
        }

        if (_Settings->_FlipHorizontally)
        {
            // Adjust the left and/or right boundary of the first and last label to make sure they're completely visible.
            if (_Labels.front().Rect.right > _RMSRect.left)
                _Labels.front().Rect.right = _RMSRect.left;

            if (_Labels.back().Rect.left < 0.f)
                _Labels.back().Rect.left = 0.f;

            // Hide overlapping labels except the first and last one.
            auto Label = &_Labels.front();

            for (size_t i = 1; i < _Labels.size() - 1; ++i)
            {
                if (_Labels[i].Rect.right > Label->Rect.left)
                    _Labels[i].IsHidden = true;
                else
                    Label = &_Labels[i];
            }

            if (_Labels.back().Rect.right > Label->Rect.left)
                Label->IsHidden = true;
        }
        else
        {
            // Adjust the left and/or right boundary of the first and last label to make sure they're completely visible.
            if (_Labels.front().Rect.left < 0.f)
                _Labels.front().Rect.left = 0.f;

            if (_Labels.back().Rect.right > _TopNameRect.right)
                _Labels.back().Rect.right = _TopNameRect.right;

            // Hide overlapping labels except the first and last one.
            auto Label = &_Labels.front();

            for (size_t i = 1; i < _Labels.size() - 1; ++i)
            {
                if (_Labels[i].Rect.left < Label->Rect.right)
                    _Labels[i].IsHidden = true;
                else
                    Label = &_Labels[i];
            }

            if (_Labels.back().Rect.left < Label->Rect.right)
                Label->IsHidden = true;
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
            D2D1_RECT_F r;

            switch (_TextAlignment)
            {
                case DWRITE_TEXT_ALIGNMENT_LEADING:
                    x1 = _Rect.left;
                    x2 = x1 + _TickSize;
                    r = { x2 + 1.f, 0.f, _Rect.right, 0.f };
                    break;

                case DWRITE_TEXT_ALIGNMENT_CENTER:
                    x1 = _Rect.left  + 1.f;
                    x2 = _Rect.right - 1.f;
                    r = { _Rect.left, 0.f, _Rect.right, 0.f };
                    break;

                case DWRITE_TEXT_ALIGNMENT_TRAILING:
                    x2 = _Rect.right;
                    x1 = x2 - _TickSize;
                    r = { _Rect.left, 0.f, x1 - 1.f, 0.f };
                    break;

                case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
                default:
                    x1 = x2 = 0.f;
            }

            const FLOAT y = msc::Map(_Settings->ScaleAmplitude(ToMagnitude(Amplitude)), 0., 1., yMin, yMax);

            r.top    = y - dh;
            r.bottom = r.top + _ScaleTextStyle->_Height;

            label_t Label =
            {
                Text, Amplitude, false,
                D2D1::Point2F(x1, y),
                D2D1::Point2F(x2, y),
                r
            };

            _Labels.push_back(Label);
        }

        if (_Settings->_FlipVertically)
        {
            // Adjust the top and/or bottom boundary of the first and last label to make sure they're completely visible.
            if (_Labels.front().Rect.top < 0.f)
                _Labels.front().Rect.top = 0.f;

            if (_Labels.back().Rect.bottom > _TopNameRect.bottom)
                _Labels.back().Rect.bottom = _TopNameRect.bottom;

            // Hide overlapping labels except the first and last one.
            auto Label = &_Labels.front();

            for (size_t i = 1; i < _Labels.size() - 1; ++i)
            {
                if (_Labels[i].Rect.top < Label->Rect.bottom)
                    _Labels[i].IsHidden = true;
                else
                    Label = &_Labels[i];
            }

            if (_Labels.back().Rect.top > Label->Rect.bottom)
                Label->IsHidden = true;

        }
        else
        {
            // Adjust the top and/or bottom boundary of the first and last label to make sure they're completely visible.
            if (_Labels.front().Rect.bottom > _RMSRect.bottom)
                _Labels.front().Rect.bottom = _RMSRect.bottom;

            if (_Labels.back().Rect.top < 0.f)
                _Labels.back().Rect.top = 0.f;

            // Hide overlapping labels except the first and last one.
            auto Label = &_Labels.front();

            for (size_t i = 1; i < _Labels.size() - 1; ++i)
            {
                if (_Labels[i].Rect.bottom > Label->Rect.top)
                    _Labels[i].IsHidden = true;
                else
                    Label = &_Labels[i];
            }

            if (_Labels.back().Rect.bottom > Label->Rect.top)
                Label->IsHidden = true;
        }
    }
}

/// <summary>
/// Unbinds the Direct2D resources from this instance.
/// </summary>
void bar_t::Unbind() noexcept
{
    _TickLinesCommandList.Release();
    _NameTextLayout.Release();

    __super::Unbind();
}

/// <summary>
/// Initializes the boundaries of this instance and prepares the transform.
/// </summary>
void bar_t::SetRect(const D2D1_RECT_F & rect) noexcept
{
    __super::SetRect(rect);

    // Prealculate the transform.
    _Transform = D2D1::Matrix3x2F::Identity();

    // Compute center of the area.
    const D2D1_POINT_2F Center = { (_Rect.left + _Rect.right) / 2.f, (_Rect.top + _Rect.bottom) / 2.f };

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

    _LEDSize = _State->_LEDLight + _State->_LEDGap;

    HRESULT hr = S_OK;

    if (_NameTextLayout == nullptr)
    {
        const FLOAT Width  = std::max(_TopNameRect.right  - _TopNameRect.left, _BottomNameRect.right  - _BottomNameRect.left);
        const FLOAT Height = std::max(_TopNameRect.bottom - _TopNameRect.top,  _BottomNameRect.bottom - _BottomNameRect.top);

        hr = _DirectWrite.Factory->CreateTextLayout(_Measurement->Name.c_str(), (UINT32) _Measurement->Name.length(), _NameStyle->_TextFormat, Width, Height, &_NameTextLayout);
    }

    if (SUCCEEDED(hr) && (_TickLinesCommandList == nullptr))
        hr = CreateTickLinesCommandList();
}

/// <summary>
/// Renders this instance.
/// </summary>
void bar_t::Render() const noexcept
{
//  _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green)); _DeviceContext->DrawRectangle(_Rect, _DebugBrush);

    D2D1_RECT_F Rect = _Rect;

    // Draw tick lines.
    _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
/*
    if (_State->_HorizontalPeakMeter)
    {
        for (const label_t & Label : _Labels)
            _DeviceContext->DrawLine(D2D1::Point2F(Label.P2.x, _Rect.top), D2D1::Point2F(Label.P2.x, _Rect.bottom), _ScaleLineStyle->_Brush, _ScaleLineStyle->_Thickness);
    }
    else
    {
        for (const label_t & Label : _Labels)
            _DeviceContext->DrawLine(D2D1::Point2F(_Rect.left, Label.P1.y), D2D1::Point2F(_Rect.right, Label.P1.y), _ScaleLineStyle->_Brush, _ScaleLineStyle->_Thickness);
    }
*/
    if (_TickLinesCommandList != nullptr)
        _DeviceContext->DrawImage(_TickLinesCommandList);

    // Draw the bars.
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
            else
            {
                if (!_State->_LEDIntegralSize)
                {
                    Rect.top    -= _LEDSize / 2.f;
                    Rect.bottom += _LEDSize / 2.f;
                }
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
            else
            {
                if (!_State->_LEDIntegralSize)
                {
                    Rect.top    -= _LEDSize / 2.f;
                    Rect.bottom += _LEDSize / 2.f;
                }
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
    if (_NameTextLayout != nullptr)
    {
        WCHAR Text[16];

        // Draw the channel name.
        if (_NameStyle->IsEnabled())
        {
            if (_Settings->_XAxisTop)
            {
//              _DeviceContext->DrawRectangle(_TopNameRect, _DebugBrush);
//              _DeviceContext->DrawText(_Measurement->Name.c_str(), (UINT) _Measurement->Name.size(), _NameStyle->_TextFormat, _TopNameRect, _NameStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                _DeviceContext->DrawTextLayout(D2D1::Point2F(_TopNameRect.left, _TopNameRect.top), _NameTextLayout, _NameStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_Settings->_XAxisBottom)
            {
//              _DeviceContext->DrawRectangle(_BottomNameRect, _DebugBrush);
//              _DeviceContext->DrawText(_Measurement->Name.c_str(), (UINT) _Measurement->Name.size(), _NameStyle->_TextFormat, _BottomNameRect, _NameStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                _DeviceContext->DrawTextLayout(D2D1::Point2F(_BottomNameRect.left, _BottomNameRect.top), _NameTextLayout, _NameStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
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
/// Creates a command list that renders the tick lines.
/// </summary>
HRESULT bar_t::CreateTickLinesCommandList() noexcept
{
    // BeginDraw() was already called by the graph. End drawing on the old target.
    HRESULT hr = _DeviceContext->EndDraw();

    if (!SUCCEEDED(hr))
        return hr;

    CComPtr<ID2D1Image> OldTarget;

    _DeviceContext->GetTarget(&OldTarget);

    hr = _DeviceContext->CreateCommandList(&_TickLinesCommandList);

    if (SUCCEEDED(hr))
    {
        _DeviceContext->SetTarget(_TickLinesCommandList);
        _DeviceContext->BeginDraw();

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

        if (_State->_HorizontalPeakMeter)
        {
            for (const label_t & Label : _Labels)
                _DeviceContext->DrawLine(D2D1::Point2F(Label.P2.x, _Rect.top), D2D1::Point2F(Label.P2.x, _Rect.bottom), _ScaleLineStyle->_Brush, _ScaleLineStyle->_Thickness);
        }
        else
        {
            for (const label_t & Label : _Labels)
                _DeviceContext->DrawLine(D2D1::Point2F(_Rect.left, Label.P1.y), D2D1::Point2F(_Rect.right, Label.P1.y), _ScaleLineStyle->_Brush, _ScaleLineStyle->_Thickness);
        }

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
        hr = _TickLinesCommandList->Close();

    // Resume drawing on the old target.
    _DeviceContext->SetTarget(OldTarget);

    _DeviceContext->BeginDraw();

    return hr;
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
/// Unbinds the Direct2D resources from this instance.
/// </summary>
void scale_t::Unbind() noexcept
{
    _AxisCommandList.Release();

    __super::Unbind();
}

/// <summary>
/// Initializes the boundaries of this instance and prepares the transform.
/// </summary>
void scale_t::SetRect(const D2D1_RECT_F & rect) noexcept
{
    __super::SetRect(rect);

    if (_AxisCommandList == nullptr)
        CreateAxisCommandList();
}

/// <summary>
/// Renders this instance.
/// </summary>
void scale_t::Render() const noexcept
{
    _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

    _DeviceContext->DrawImage(_AxisCommandList);
/*
    _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

//  _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow)); _DeviceContext->DrawRectangle(_Rect, _DebugBrush);

    _ScaleTextStyle->SetHorizontalAlignment(_TextAlignment);
    _ScaleTextStyle->SetVerticalAlignment(_ParagraphAlignment);

    for (const label_t & Label : _Labels)
    {
//      _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red)); _DeviceContext->DrawRectangle(Label.Rect, _DebugBrush);

        if (!Label.IsHidden)
            _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _ScaleTextStyle->_TextFormat, Label.Rect, _ScaleTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

        if (!IsCenter())
            _DeviceContext->DrawLine(Label.P1, Label.P2, _ScaleLineStyle->_Brush, _ScaleLineStyle->_Thickness);
    }
*/
}

/// <summary>
/// Creates a command list that renders the axis.
/// </summary>
HRESULT scale_t::CreateAxisCommandList() noexcept
{
    // BeginDraw() was already called by the graph. End drawing on the old target.
    HRESULT hr = _DeviceContext->EndDraw();

    if (!SUCCEEDED(hr))
        return hr;

    CComPtr<ID2D1Image> OldTarget;

    _DeviceContext->GetTarget(&OldTarget);

    hr = _DeviceContext->CreateCommandList(&_AxisCommandList);

    if (SUCCEEDED(hr))
    {
        _DeviceContext->SetTarget(_AxisCommandList);
        _DeviceContext->BeginDraw();

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

    //  _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow)); _DeviceContext->DrawRectangle(_Rect, _DebugBrush);

        _ScaleTextStyle->SetHorizontalAlignment(_TextAlignment);
        _ScaleTextStyle->SetVerticalAlignment(_ParagraphAlignment);

        for (const label_t & Label : _Labels)
        {
    //      _DebugBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red)); _DeviceContext->DrawRectangle(Label.Rect, _DebugBrush);

            if (!Label.IsHidden)
                _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _ScaleTextStyle->_TextFormat, Label.Rect, _ScaleTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

            if (!IsCenter())
                _DeviceContext->DrawLine(Label.P1, Label.P2, _ScaleLineStyle->_Brush, _ScaleLineStyle->_Thickness);
        }

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
        hr = _AxisCommandList->Close();

    // Resume drawing on the old target.
    _DeviceContext->SetTarget(OldTarget);

    _DeviceContext->BeginDraw();

    return hr;
}

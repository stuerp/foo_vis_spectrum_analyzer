
/** $VER: Style.cpp (2024.03.10) P. Stuer **/

#include "Style.h"

#include "Direct2D.h"
#include "DirectWrite.h"
#include "Gradients.h"

#include "Support.h"
#include "Log.h"

#include <cassert>

#pragma hdrstop

/// <summary>
/// Initializes an instance.
/// </summary>
Style::Style(const Style & other)
{
    operator=(other);
}

/// <summary>
/// Implements the = operator.
/// </summary>
Style & Style::operator=(const Style & other)
{
    _Flags = other._Flags;

    _ColorSource = other._ColorSource;
    _ColorIndex = other._ColorIndex;
    _ColorScheme = other._ColorScheme;

    _CustomColor = other._CustomColor;
    _CustomGradientStops = other._CustomGradientStops;

    _Opacity = other._Opacity;
    _Thickness = other._Thickness;

    _FontName = other._FontName;
    _FontSize = other._FontSize;

    _Color = other._Color;
    _GradientStops = other._GradientStops;

    return *this;
}

/// <summary>
/// Initializes an instance.
/// </summary>
Style::Style(uint64_t flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, GradientStops customGradientStops, FLOAT opacity, FLOAT thickness, const wchar_t * fontName, FLOAT fontSize)
{
    _Flags = flags;

    _ColorSource = colorSource;
    _ColorIndex = colorIndex;
    _ColorScheme = colorScheme;

    _CustomColor = customColor;
    _CustomGradientStops = customGradientStops;

    _Opacity = opacity;
    _Thickness = thickness;

    _FontName = fontName;
    _FontSize = fontSize;

    _Color = customColor;
    _GradientStops = (_ColorScheme == ColorScheme::Custom) ? _CustomGradientStops : GetGradientStops(_ColorScheme);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Style::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const D2D1_SIZE_F & size) noexcept
{
    HRESULT hr = S_OK;

    if (_ColorSource != ColorSource::Gradient)
        hr = renderTarget->CreateSolidColorBrush(_Color, (ID2D1SolidColorBrush **) &_Brush);
    else
    {
        if (_Flags &= Style::AmplitudeBasedColor)
            hr = renderTarget->CreateSolidColorBrush(_Color, (ID2D1SolidColorBrush **) &_Brush);
        else
            hr = _Direct2D.CreateGradientBrush(renderTarget, _GradientStops, size, _Flags & Style::HorizontalGradient, (ID2D1LinearGradientBrush **) &_Brush);
    }

    if (_Brush)
        _Brush->SetOpacity(_Opacity);

    if ((_Flags & SupportsFont) && (_TextFormat == nullptr))
    {
        const FLOAT FontSize = ToDIPs(_FontSize); // In DIPs

        hr = _DirectWrite.Factory->CreateTextFormat(_FontName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Style::ReleaseDeviceSpecificResources()
{
    _TextFormat.Release();
    _Brush.Release();
}

/// <summary>
/// Sets the color of a solid color brush from the gradient colors based on a value between 0 and 1.
/// </summary>
HRESULT Style::SetBrushColor(double value) noexcept
{
    ID2D1SolidColorBrush * ColorBrush = nullptr;

    HRESULT hr = _Brush->QueryInterface(IID_PPV_ARGS(&ColorBrush));

    if (SUCCEEDED(hr))
    {
        D2D1_COLOR_F Color(0);

        for (auto & gs : _GradientStops)
        {
            if (value > (1. - gs.position))
                break;

            Color = gs.color;
        }

        ColorBrush->SetColor(Color);

        ColorBrush->Release();
    }

    return hr;
}

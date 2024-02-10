
/** $VER: Style.cpp (2024.02.10) P. Stuer **/

#include "Style.h"

#include "Direct2D.h"
#include "Gradients.h"

#include "Log.h"

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
    _Name = other._Name;
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
Style::Style(const char * name, uint64_t flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, GradientStops customGradientStops, FLOAT opacity, FLOAT thickness, const char * fontName, FLOAT fontSize)
{
    _Name = name;
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
HRESULT Style::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    if (_ColorSource != ColorSource::Gradient)
        hr = renderTarget->CreateSolidColorBrush(_Color, (ID2D1SolidColorBrush **) &_Brush);
    else
        hr = _Direct2D.CreateGradientBrush(renderTarget, _GradientStops, _Flags & Style::HorizontalGradient, (ID2D1LinearGradientBrush **) &_Brush);

    if (_Brush)
        _Brush->SetOpacity(_Opacity);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Style::ReleaseDeviceSpecificResources()
{
    _Brush.Release();
}

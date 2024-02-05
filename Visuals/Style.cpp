
/** $VER: Style.cpp (2024.02.05) P. Stuer **/

#include "Style.h"

#pragma hdrstop

Style::Style(const Style & other)
{
    operator=(other);
}

Style & Style::operator=(const Style & other)
{
    _Name = other._Name;

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

    _Brush.Release();

    return *this;
}

Style::Style(const char * name, ColorSource colorSource, D2D1_COLOR_F customColor, int colorIndex, ColorScheme colorScheme, GradientStops customGradientStops, FLOAT opacity, FLOAT thickness, const char * fontName, FLOAT fontSize)
{
    _Name = name;

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
    _GradientStops = customGradientStops;
}

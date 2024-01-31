
/** $VER: Style.h (2024.01.31) P. Stuer - Represents the style of a visual element. **/

#pragma once

#include "framework.h"

#include "Configuration.h"

enum class VisualElement
{
    Background,
    XAxisText,
    XAxisLine,
    YAxisText,
    YAxisLine,

    BarForeground,
    PeakIndicator,
    BarWhiteKey,
    BarBlackKey,

    CurveLine,
    CurveArea,
    PeakLine,
    PeakArea
};

enum class ColorSource
{
    Solid,
    DominantColor,
    Gradient,
    Windows,
    Host,
};

class Style
{
public:
    std::wstring _Name;

    ColorSource _ColorSource;           // Determines the source of the color.
    D2D1_COLOR_F _CustomColor;          // User-specified color
    ColorScheme _ColorScheme;           // User-specified color scheme
    GradientStops _CustomGradientStops; // User-specified gradient stops

    // Area-specific
    FLOAT _Opacity;                     // Area opacity

    // Line-specific
    FLOAT _Thickness;                   // Line thickness

    // Font-specific
    std::wstring _FontName;
    FLOAT _FontSize;

    // Resulting resources
    D2D1_COLOR_F _Color;
    GradientStops _GradientStops;

    // DirectX resources
    CComPtr<ID2D1Brush> _Brush;
};

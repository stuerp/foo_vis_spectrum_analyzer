
/** $VER: Style.h (2024.01.31) P. Stuer - Represents the style of a visual element. **/

#pragma once

#include "framework.h"

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
    Gradient,
    Windows,
    Host,
};

typedef std::vector<D2D1_GRADIENT_STOP> GradientStops;

class Style
{
public:
    std::wstring _Name;

    ColorSource _ColorSource;       // Determine the source of the color.
    D2D1_COLOR_F _Color;
    GradientStops _GradientStops;

    // Area-specific
    FLOAT _Opacity;                 // Area opacity

    // Line-specific
    FLOAT _Thickness;               // Line thickness

    // Font-specific
    std::wstring _FontName;
    FLOAT _FontSize;
};
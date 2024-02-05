
/** $VER: Style.h (2024.02.05) P. Stuer - Represents the style of a visual element. **/

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
    BarPeakIndicator,
    BarDarkBackground,
    BarLightBackground,

    CurveLine,
    CurveArea,
    CurvePeakLine,
    CurvePeakArea
};

enum class ColorSource
{
    None,
    Solid,
    DominantColor,
    Gradient,
    Windows,
    UserInterface,
};

enum class WindowsColor
{
    WindowBackground,           // COLOR_WINDOW
    WindowText,                 // COLOR_WINDOWTEXT
    ButtonBackground,           // COLOR_3DFACE
    ButtonText,                 // COLOR_BTNTEXT
    HighlightBackground,        // COLOR_HIGHLIGHT
    HighlightText,              // COLOR_HIGHLIGHTTEXT
    GrayText,                   // COLOR_GRAYTEXT
    HotLight,                   // COLOR_HOTLIGHT
};

enum class DUIColor
{
    Text,
    Background,
    Highlight,
    Selection,
    DarkMode
};

enum class CUIColor
{
    Text,                       // cui::colours::colour_text
    SelectedText,               // cui::colours::colour_selection_text
    InactiveSelectedText,       // cui::colours::colour_inactive_selection_text

    Background,                 // cui::colours::colour_background
    SelectedBackground,         // cui::colours::colour_selection_background
    InactiveSelectedBackground, // cui::colours::colour_inactive_selection_background

    ActiveItem,                 // cui::colours::colour_active_item_frame
};

class Style
{
public:
    Style() { }

    Style(const Style &);
    Style & operator=(const Style & other);

    virtual ~Style()
    {
        _Brush.Release();
    }

    Style(const char * name, uint64_t flags, ColorSource colorSource, D2D1_COLOR_F customColor, int colorIndex, ColorScheme colorScheme, GradientStops customGradientStops, FLOAT opacity, FLOAT thickness, const char * fontName, FLOAT fontSize);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources();

public:
    pfc::string _Name;
    uint64_t _Flags;

    ColorSource _ColorSource;           // Determines the source of the color
    D2D1_COLOR_F _CustomColor;          // User-specified color
    int _ColorIndex;                    // User-specified color index in Windows / DUI / CUI list
    ColorScheme _ColorScheme;           // User-specified color scheme
    GradientStops _CustomGradientStops; // User-specified gradient stops

    FLOAT _Opacity;                     // Opacity of the brush or area
    FLOAT _Thickness;                   // Line thickness

    // Font-specific
    pfc::string _FontName;
    FLOAT _FontSize;

    // Resulting resources
    D2D1_COLOR_F _Color;
    GradientStops _GradientStops;

    // DirectX resources
    CComPtr<ID2D1Brush> _Brush;

    enum Feature
    {
        SupportsOpacity     = 0x01,
        SupportsThickness   = 0x02,
        SupportsFont        = 0x04,

        HorizontalGradient  = 0x08,
    };
};

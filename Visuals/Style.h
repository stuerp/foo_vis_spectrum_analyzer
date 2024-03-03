
/** $VER: Style.h (2024.02.26) P. Stuer - Represents the style of a visual element. **/

#pragma once

#include "framework.h"

enum class VisualElement : uint32_t
{
    GraphBackground             =  0,
    GraphDescriptionText        =  1,
    GraphDescriptionBackground  = 14,

    XAxisText                   =  2,
    VerticalGridLine            =  3,
    YAxisText                   =  4,
    HorizontalGridLine          =  5,

    BarSpectrum                 =  6,
    BarPeakIndicator            =  7,
    BarDarkBackground           =  8,
    BarLightBackground          =  9,

    CurveLine                   = 10,
    CurveArea                   = 11,
    CurvePeakLine               = 12,
    CurvePeakArea               = 13,

    NyquistMarker               = 15,
};

enum class ColorSource : uint32_t
{
    None,
    Solid,
    DominantColor,
    Gradient,
    Windows,
    UserInterface,
};

enum class WindowsColor : uint32_t
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

enum class DUIColor : uint32_t
{
    Text,
    Background,
    Highlight,
    Selection,
    DarkMode
};

enum class CUIColor : uint32_t
{
    Text,                       // cui::colours::colour_text
    SelectedText,               // cui::colours::colour_selection_text
    InactiveSelectedText,       // cui::colours::colour_inactive_selection_text

    Background,                 // cui::colours::colour_background
    SelectedBackground,         // cui::colours::colour_selection_background
    InactiveSelectedBackground, // cui::colours::colour_inactive_selection_background

    ActiveItem,                 // cui::colours::colour_active_item_frame
};

enum class ColorScheme : uint32_t
{
    Solid = 0,
    Custom = 1,
    Artwork = 2,

    Prism1 = 3,
    Prism2 = 4,
    Prism3 = 5,

    foobar2000 = 6,
    foobar2000DarkMode = 7,

    Fire = 8,
    Rainbow = 9,
};

class Style
{
public:
    Style() { }

    Style(const Style &);
    Style & operator=(const Style & other);

    virtual ~Style() { }

    Style(uint64_t flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, GradientStops customGradientStops, FLOAT opacity, FLOAT thickness, const char * fontName, FLOAT fontSize);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const D2D1_SIZE_F & size) noexcept;
    void ReleaseDeviceSpecificResources();

public:
    uint64_t _Flags;

    ColorSource _ColorSource;           // Determines the source of the color
    D2D1_COLOR_F _CustomColor;          // User-specified color
    uint32_t _ColorIndex;               // User-specified color index in Windows / DUI / CUI list
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

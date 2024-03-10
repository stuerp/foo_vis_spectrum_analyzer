
/** $VER: Style.h (2024.03.10) P. Stuer - Represents the style of a visual element. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <atlbase.h>

#include "Gradients.h"

#include <string>

#pragma warning(disable: 4820)
class Style
{
public:
    Style() { }

    Style(const Style &);
    Style & operator=(const Style & other);

    virtual ~Style() { }

    Style(uint64_t flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, GradientStops customGradientStops, FLOAT opacity, FLOAT thickness, const wchar_t * fontName, FLOAT fontSize);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const D2D1_SIZE_F & size) noexcept;
    void ReleaseDeviceSpecificResources();

    HRESULT SetBrushColor(double value) noexcept;

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
    std::wstring _FontName;
    FLOAT _FontSize;

    // Current input value for the DirectX resources
    D2D1_COLOR_F _CurrentColor;
    GradientStops _CurrentGradientStops;

    // DirectX resources
    CComPtr<ID2D1Brush> _Brush;
    CComPtr<IDWriteTextFormat> _TextFormat;

    enum Feature
    {
        SupportsOpacity     = 0x01,
        SupportsThickness   = 0x02,
        SupportsFont        = 0x04,

        HorizontalGradient  = 0x08,
        AmplitudeBasedColor = 0x10,

        System              = SupportsOpacity | SupportsThickness | SupportsFont,
    };
};

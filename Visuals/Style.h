
/** $VER: Style.h (2024.04.20) P. Stuer - Represents the style of a visual element. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <atlbase.h>

#include "Gradients.h"

#include "DirectWrite.h"

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

    bool IsEnabled() const noexcept { return (_ColorSource != ColorSource::None); }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const std::wstring & text, const D2D1_SIZE_F & size) noexcept;
    void ReleaseDeviceSpecificResources();

    HRESULT MeasureText(const std::wstring & text) noexcept;

    HRESULT SetBrushColor(double value) noexcept;
    void UpdateCurrentColor(const D2D1_COLOR_F & dominantColor, const std::vector<D2D1_COLOR_F> & userInterfaceColors);

    void SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT ta) const noexcept
    {
        if (_TextFormat)
            _TextFormat->SetTextAlignment(ta);
    }

    void SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT pa) const noexcept
    {
        if (_TextFormat)
            _TextFormat->SetParagraphAlignment(pa);
    }

    FLOAT GetWidth() const noexcept { return _Width; }
    FLOAT GetHeight() const noexcept { return _Height; }

    static HRESULT CreateAmplitudeMap(ColorScheme colorScheme, const GradientStops & gradientStops, std::vector<D2D1_COLOR_F> & colors) noexcept;

private:
    static D2D1_COLOR_F GetWindowsColor(uint32_t index) noexcept;

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
    std::vector<D2D1_COLOR_F> _AmplitudeMap;

    FLOAT _Width;
    FLOAT _Height;

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

        AmplitudeAware      = 0x20,

        System              = SupportsOpacity | SupportsThickness | SupportsFont | AmplitudeAware,
    };
};


/** $VER: Style.h (2025.09.17) P. Stuer - Represents the style of a visual element. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <atlbase.h>

#include "Gradients.h"

#include "DirectWrite.h"
#include "Support.h"

#include <string>

#pragma warning(disable: 4820)
class style_t
{
public:
    style_t() { }

    style_t(const style_t &);
    style_t & operator=(const style_t & other);

    virtual ~style_t() { }

    enum class Features : uint64_t
    {
        SupportsOpacity     = 0x01,
        SupportsThickness   = 0x02,
        SupportsFont        = 0x04,

        HorizontalGradient  = 0x08,
        AmplitudeBasedColor = 0x10,

        AmplitudeAware      = 0x20,

        SupportsRadial      = 0x40,
        RadialGradient      = 0x80,

        System              = SupportsOpacity | SupportsThickness | SupportsFont | AmplitudeAware | SupportsRadial,
    };

    style_t(Features flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, gradient_stops_t customGradientStops, FLOAT opacity, FLOAT thickness, const wchar_t * fontName, FLOAT fontSize);

    bool IsEnabled() const noexcept { return (_ColorSource != ColorSource::None); }

    bool Has(Features feature) const noexcept { return IsSet(_Flags, feature); }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const std::wstring & text, const D2D1_SIZE_F & size) noexcept;
    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset) noexcept;
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

    static HRESULT CreateAmplitudeMap(ColorScheme colorScheme, const gradient_stops_t & gradientStops, std::vector<D2D1_COLOR_F> & colors) noexcept;

private:
    static D2D1_COLOR_F GetWindowsColor(uint32_t index) noexcept;

public:
    Features _Flags;

    ColorSource _ColorSource;           // Determines the source of the color
    D2D1_COLOR_F _CustomColor;          // User-specified color
    uint32_t _ColorIndex;               // User-specified color index in Windows / DUI / CUI list
    ColorScheme _ColorScheme;           // User-specified color scheme
    gradient_stops_t _CustomGradientStops; // User-specified gradient stops

    FLOAT _Opacity;                     // Opacity of the brush or area
    FLOAT _Thickness;                   // Line thickness

    // Font-specific
    std::wstring _FontName;
    FLOAT _FontSize;

    // Current input value for the DirectX resources
    D2D1_COLOR_F _CurrentColor;
    gradient_stops_t _CurrentGradientStops;
    std::vector<D2D1_COLOR_F> _AmplitudeMap;

    // DirectX resources
    CComPtr<ID2D1Brush> _Brush;
    CComPtr<IDWriteTextFormat> _TextFormat;

    FLOAT _Width;
    FLOAT _Height;
};

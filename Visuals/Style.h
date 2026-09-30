
/** $VER: Style.h (2026.09.29) P. Stuer - Represents the style of a visual element. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <atlcomcli.h>

#include <dwrite.h>
#include <string>

#include "Gradient.h"

class state_t;

class style_t
{
public:
    style_t() = default;

    style_t(const style_t &) noexcept;
    style_t & operator=(const style_t & other) noexcept;

    style_t(const style_t &&) = delete;
    style_t & operator=(const style_t && other) = delete;

    virtual ~style_t() = default;

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

        Global              = 1ull << 63,

        Min = SupportsOpacity,
        Max = Global,
    };

    style_t(const std::wstring & name, VisualizationTypes usedBy, Features flags, ColorSource colorSource, D2D1_COLOR_F customColor, uint32_t colorIndex, ColorScheme colorScheme, std::vector<D2D1_GRADIENT_STOP> customGradientStops, FLOAT opacity, FLOAT thickness, const wchar_t * fontName, FLOAT fontSize) noexcept;

    bool IsEnabled() const noexcept
    {
        return (_ColorSource != ColorSource::None);
    }

    bool Has(Features feature) const noexcept
    {
        return IsSet(_Flags, feature);
    }

    void SetColor(const state_t * state) noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const std::wstring & text, FLOAT scaleFactor = 1.f) noexcept;
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT MeasureText(const std::wstring & text) noexcept;

    HRESULT SetBrushColor(double value) noexcept;

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

    bool IsAmplitudeBased() const noexcept { return (_ColorSource == ColorSource::Gradient) && Has(style_t::Features::HorizontalGradient | style_t::Features::AmplitudeBasedColor); }

    static HRESULT CreateAmplitudeMap(ColorScheme colorScheme, const std::vector<D2D1_GRADIENT_STOP> & gradientStops, std::vector<D2D1_COLOR_F> & colors) noexcept;

private:
    static D2D1_COLOR_F GetWindowsColor(uint32_t index) noexcept;

public:
    std::wstring _Name;
    VisualizationTypes _UsedBy;                     // Determines which visualization uses the style.

#pragma region Serialized

    Features _Flags;

    ColorSource _ColorSource;                       // The source of the color
    uint32_t _ColorIndex;                           // The index in the Windows or user interface color list
    ColorScheme _ColorScheme;                       // The selected gradient color scheme

    D2D1_COLOR_F _CustomColor;                      // User-specified color
    std::vector<gradient_stop_t> _CustomGradient;   // User-specified gradient stops

    FLOAT _Opacity;                                 // Opacity of the brush or area
    FLOAT _Thickness;                               // Line thickness

    std::wstring _FontName;
    FLOAT _FontSize;

#pragma endregion

    // Current input value for the DirectX resources
    D2D1_COLOR_F _CurrentColor;
    std::vector<D2D1_GRADIENT_STOP> _CurrentGradientStops;
    std::vector<D2D1_COLOR_F> _AmplitudeMap;

    // DirectX resources
    ComPtr<ID2D1Brush> _Brush;
    ComPtr<IDWriteTextFormat> _TextFormat;

    FLOAT _Width;
    FLOAT _Height;
};

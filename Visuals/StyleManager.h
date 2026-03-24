
/** $VER: StyleManager.h (2026.03.21) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "pch.h"

#include "Style.h"
#include "Gradients.h"

#pragma warning(disable: 4868) // compiler may not enforce left-to-right evaluation order in braced initializer list

#include <nlohmann\json.hpp>

using json = nlohmann::ordered_json;

#pragma warning(default: 4868)

#pragma warning(disable: 4820)

class style_manager_t
{
public:
    style_manager_t();

    style_manager_t(const style_manager_t &) = default;

    style_manager_t & operator=(const style_manager_t & other);

    virtual ~style_manager_t() noexcept { }

    void Reset() noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

    json ToJSON() const noexcept;
    void FromJSON(const json & array) noexcept;

    /// <summary>
    /// Gets the style of the specified visual element.
    /// </summary>
    style_t * GetStyle(VisualElement visualElement) noexcept
    {
        return &Styles[visualElement];
    }

    /// <summary>
    /// Gets the default style of the specified visual element.
    /// </summary>
    const style_t * GetDefaultStyle(VisualElement visualElement) noexcept
    {
        return &Styles[visualElement];
    }

    void SetArtworkDependentParameters(const gradient_stops_t & gs, D2D1_COLOR_F dominantColor) noexcept;

    void UpdateCurrentColors() noexcept;

    void DeleteGradientBrushes() noexcept
    {
        for (auto & [ID, Style] : Styles)
        {
            if (Style._ColorSource == ColorSource::Gradient)
                Style._Brush.Release();
        }
    }

    HRESULT GetInitializedStyle(VisualElement visualElement, ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const std::wstring & text, FLOAT scaleFactor, style_t ** style) noexcept;

    HRESULT GetInitializedStyle(VisualElement visualElement, ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, style_t ** style) noexcept;

    void DeleteDeviceSpecificResources() noexcept;

private:
    static json ToJSON(const gradient_stops_t & gradientStops) noexcept;
    static gradient_stops_t FromJSONGradientStops(const json::array_t & array) noexcept;

    static json ToJSON(const D2D1_GRADIENT_STOP & gs) noexcept;
    static D2D1_GRADIENT_STOP FromJSONGradientStop(const json & object) noexcept;

    static json ToJSON(const D2D1_COLOR_F & color) noexcept;
    static D2D1_COLOR_F FromJSONColor(const json & object) noexcept;

public:
    std::unordered_map<VisualElement, style_t> Styles;

    std::vector<D2D1_COLOR_F> UserInterfaceColors;
    D2D1_COLOR_F DominantColor;                     // The current dominant color extracted from the artwork bitmap.

    // An index of the styles to determine the display order in the configuration dialog.
    const std::vector<VisualElement> DisplayOrder =
    {
        VisualElement::GraphBackground,
        VisualElement::GraphDescriptionText,
        VisualElement::GraphDescriptionBackground,

        VisualElement::XAxisText,
        VisualElement::XAxisLine,

        VisualElement::YAxisText,
        VisualElement::YAxisLine,

        VisualElement::HorizontalGridLine,
        VisualElement::VerticalGridLine,

        VisualElement::NyquistMarker,

        // Spectrum Analyzer
        VisualElement::BarArea,
        VisualElement::BarTop,

        VisualElement::BarPeakArea,
        VisualElement::BarPeakTop,

        VisualElement::BarDarkBackground,
        VisualElement::BarLightBackground,

        VisualElement::CurveLine,
        VisualElement::CurveArea,
        VisualElement::CurvePeakLine,
        VisualElement::CurvePeakArea,

        // Spectrogram
        VisualElement::Spectrogram,

        // Peak Meter
        VisualElement::BarBackground,
        VisualElement::BarPeakLevel,
        VisualElement::Bar0dBPeakLevel,
        VisualElement::BarMaxPeakLevel,
        VisualElement::BarPeakLevelText,
        VisualElement::BarRMSLevel,
        VisualElement::Bar0dBRMSLevel,
        VisualElement::BarRMSLevelText,

        // Level Meter
        VisualElement::BarLeftRight,
        VisualElement::BarLeftRightIndicator,
        VisualElement::BarMidSide,
        VisualElement::BarMidSideIndicator,
        VisualElement::LevelMeterAxis,

        // Oscilloscope
        VisualElement::SignalLine,

        // Bit Meter
        VisualElement::BarSign,
        VisualElement::BarExponent,
        VisualElement::BarMantissa,
    };

private:
    #pragma warning(disable: 4868)
    std::unordered_map<VisualElement, style_t> _DefaultStyles
    {
        #pragma region Common
        {
            VisualElement::GraphBackground,
            {
                /* Name                */ L"Graph Background",
                /* UsedBy              */ VisualizationTypes::All,
                /* Flags               */ style_t::Features::SupportsOpacity,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF(D2D1::ColorF::Black),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Solid,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f
            }
        },

        {
            VisualElement::GraphDescriptionText,
            {
                /* Name                */ L"Graph Description Text",
                /* UsedBy              */ VisualizationTypes::All,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF(D2D1::ColorF::White),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Solid,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"Segoe UI",
                /* FontSize            */ 14.f,
            }
        },

        {
            VisualElement::GraphDescriptionBackground,
            {
                /* Name                */ L"Graph Description Background",
                /* UsedBy              */ VisualizationTypes::All,
                /* Flags               */ style_t::Features::SupportsOpacity,
                /* ColorSource         */ ColorSource::Solid, D2D1::ColorF(1.f, 1.f, 1.f, .25f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::XAxisText,
            {
                /* Name                */ L"X-axis Text",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectrogram | VisualizationTypes::PeakMeter | VisualizationTypes::Oscilloscope | VisualizationTypes::BitMeter,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF(D2D1::ColorF::White),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Solid,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"Segoe UI",
                /* FontSize            */ 6.f
            }
        },

        {
            VisualElement::XAxisLine,
            {
                /* Name                */ L"X-axis Line",
                /* UsedBy              */ VisualizationTypes::Oscilloscope,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF(.25f, .25f, .25f, 1.f),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Solid,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /* Opacity             */ 1.f,
                /* Thickness           */ 1.f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f,
            }
        },

        {
            VisualElement::YAxisText,
            {
                /* Name                */ L"Y-axis Text",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectrogram | VisualizationTypes::PeakMeter | VisualizationTypes::Oscilloscope | VisualizationTypes::BitMeter,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF(D2D1::ColorF::White),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Solid,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"Segoe UI",
                /* FontSize            */ 6.f
            }
        },

        {
            VisualElement::YAxisLine,
            {
                /* Name                */ L"Y-axis Line",
                /* UsedBy              */ VisualizationTypes::Oscilloscope,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /* ColorSource         */ ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::HorizontalGridLine,
            {
                /* Name                */ L"Horizontal Grid Line",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::PeakMeter | VisualizationTypes::Spectrogram | VisualizationTypes::Oscilloscope,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /* CustomColor         */ ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::VerticalGridLine,
            {
                /* Name                */ L"Vertical Grid Line",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectrogram | VisualizationTypes::Oscilloscope,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /* CustomColor         */ ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::NyquistMarker,
            {
                /* Name                */ L"Nyquist Frequency Line",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectrogram,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /* CustomColor         */ ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },
        #pragma endregion

        #pragma region Spectrum Bars, Radial Bars
        {
            VisualElement::BarArea,
            {
                /* Name                */ L"Bar Area",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::RadialBars,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware,
                /* CustomColor         */ ColorSource::Gradient,
                D2D1::ColorF(D2D1::ColorF::Black),
                0,
                ColorScheme::Prism1,
                GetBuiltInGradientStops(ColorScheme::Custom),
                1.f,
                0.f,
                L"",
                0.f,
            }
        },

        {
            VisualElement::BarTop,
            {
                /* Name                */ L"Bar Top",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::RadialBars,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /* CustomColor         */ ColorSource::None, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 5.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarPeakArea,
            {
                /* Name                */ L"Bar Peak Area",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::RadialBars,
                style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware,
                ColorSource::None, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 0.25f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarPeakTop,
            {
                /* Name                */ L"Bar Peak Top",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::RadialBars,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarDarkBackground,
            {
                /* Name                */ L"Bar Dark Background",
                /* UsedBy              */ VisualizationTypes::Bars,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarLightBackground,
            {
                /* Name                */ L"Bar Light Background",
                /* UsedBy              */ VisualizationTypes::Bars,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },
        #pragma endregion

        #pragma region Curve, Radial Curve
        {
            VisualElement::CurveLine,
            {
                /* Name                */ L"Curve Line",
                /* UsedBy              */ VisualizationTypes::Curve | VisualizationTypes::RadialCurve,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 2.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurveArea,
            {
                /* Name                */ L"Curve Area",
                /* UsedBy              */ VisualizationTypes::Curve | VisualizationTypes::RadialCurve,
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), .5f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurvePeakLine,
            {
                /* Name                */ L"Curve Peak Line",
                /* UsedBy              */ VisualizationTypes::Curve | VisualizationTypes::RadialCurve,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 2.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurvePeakArea,
            {
                /* Name                */ L"Curve Peak Area",
                /* UsedBy              */ VisualizationTypes::Curve | VisualizationTypes::RadialCurve,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), .25f, 0.f, L"", 0.f,
            }
        },
        #pragma endregion

        #pragma region Spectrogram
        {
            VisualElement::Spectrogram,
            {
                /* Name                */ L"Spectrogram",
                /* UsedBy              */ VisualizationTypes::Spectrogram,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware | style_t::Features::AmplitudeBasedColor | style_t::Features::HorizontalGradient,
                /* ColorSource         */ ColorSource::Gradient,
                /* CustomColor         */ D2D1::ColorF(D2D1::ColorF::Black),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::SoX,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f
            }
        },
        #pragma endregion

        #pragma region Peak Meter
        {
            VisualElement::BarBackground,
            {
                /* Name                */ L"Bar Background",
                /* UsedBy              */ VisualizationTypes::PeakMeter | VisualizationTypes::BitMeter,
                /* Flags               */ style_t::Features::SupportsOpacity,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF(.2f, .2f, .2f, 1.f),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Solid,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f,
            }
        },

        {
            VisualElement::BarPeakLevel,
            {
                /* Name                */ L"Peak Level",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient,
                D2D1::ColorF(D2D1::ColorF::Black),
                0,
                ColorScheme::Prism1,
                GetBuiltInGradientStops(ColorScheme::Custom),
                1.f,
                0.f,
                L"",
                0.f,
            }
        },

        {
            VisualElement::Bar0dBPeakLevel,
            {
                /* Name                */ L"Peak Level (> 0dB)",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarMaxPeakLevel,
            {
                /* Name                */ L"Peak Level (Max)",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarPeakLevelText,
            {
                /* Name                */ L"Peak Level Read Out",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 10.f,
            }
        },

        // RMS
        {
            VisualElement::BarRMSLevel,
            {
                /* Name                */ L"RMS Level",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Bar0dBRMSLevel,
            {
                /* Name                */ L"RMS Level (> 0dB)",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarRMSLevelText,
            {
                /* Name                */ L"RMS Level Read Out",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 10.f,
            }
        },
        #pragma endregion

        #pragma region Level Meter
        {
            VisualElement::BarLeftRight,
            {
                /* Name                */ L"Left/Right Level",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarLeftRightIndicator,
            {
                /* Name                */ L"Left/Right Level Indicator",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarMidSide,
            {
                /* Name                */ L"Mid/Side Level",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarMidSideIndicator,
            {
                /* Name                */ L"Mid/Side Level Indicator",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::LevelMeterAxis,
            {
                /* Name                */ L"Left/Side Axis",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 0.5f, 1.f, L"Segoe UI", 10.f,
            }
        },
        #pragma endregion

        #pragma region Oscilloscope
        {
            VisualElement::SignalLine,
            {
                /* Name                */ L"Signal Line",
                /* UsedBy              */ VisualizationTypes::Oscilloscope,
                /* Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF(104.f/255.f, 208.f/255.f, 208.f/255.f, 1.f),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Solid,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Solid),
                /* Opacity             */ 1.f,
                /* Thickness           */ 1.5f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f
            }
        },
        #pragma endregion

        #pragma region Bit Meter
        {
            VisualElement::BarSign,
            {
                /* Name                */ L"Sign Bits",
                /* UsedBy              */ VisualizationTypes::BitMeter,
                /* Flags               */ style_t::Features::SupportsOpacity,
                /* ColorSource         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF((UINT32) RGB(192, 192, 192)),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Prism1,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Prism1),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f,
            }
        },
        {
            VisualElement::BarMantissa,
            {
                /* Name                */ L"Mantissa Bits",
                /* UsedBy              */ VisualizationTypes::BitMeter,
                /* Flags               */ style_t::Features::SupportsOpacity,
                /* CustomColor         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF((UINT32) RGB(86, 156, 214)),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Prism1,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Prism1),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f,
            }
        },
        {
            VisualElement::BarExponent,
            {
                /* Name                */ L"Exponent Bits",
                /* UsedBy              */ VisualizationTypes::BitMeter,
                /* Flags               */ style_t::Features::SupportsOpacity,
                /* CustomColor         */ ColorSource::Solid,
                /* CustomColor         */ D2D1::ColorF((UINT32) RGB(214, 156, 86)),
                /* ColorIndex          */ 0,
                /* ColorScheme         */ ColorScheme::Prism1,
                /* CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Prism1),
                /* Opacity             */ 1.f,
                /* Thickness           */ 0.f,
                /* FontName            */ L"",
                /* FontSize            */ 0.f,
            }
        },
        #pragma endregion
    };

    const uint32_t _CurrentVersion = 5;
};

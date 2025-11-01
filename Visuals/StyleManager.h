
/** $VER: StyleManager.h (2025.10.14) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "pch.h"

#include "Style.h"
#include "Gradients.h"

#include <map>

#pragma warning(disable: 4820)
class style_manager_t
{
public:
    style_manager_t();

    style_manager_t(const style_manager_t &) = delete;
    style_manager_t & operator=(const style_manager_t & other);
    style_manager_t(style_manager_t &&) = delete;
    style_manager_t & operator=(style_manager_t &&) = delete;

    virtual ~style_manager_t() { }

    void Reset() noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

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

        // Spectogram
        VisualElement::Spectogram,

        // Peak Meter
        VisualElement::GaugeBackground,
        VisualElement::GaugePeakLevel,
        VisualElement::Gauge0dBPeakLevel,
        VisualElement::GaugeMaxPeakLevel,
        VisualElement::GaugePeakLevelText,
        VisualElement::GaugeRMSLevel,
        VisualElement::Gauge0dBRMSLevel,
        VisualElement::GaugeRMSLevelText,

        // Level Meter
        VisualElement::GaugeLeftRight,
        VisualElement::GaugeLeftRightIndicator,
        VisualElement::GaugeMidSide,
        VisualElement::GaugeMidSideIndicator,
        VisualElement::LevelMeterAxis,

        // Oscilloscope
        VisualElement::SignalLine,
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
                /*_Flags               */ style_t::Features::SupportsOpacity,
                /*_ColorSource         */ ColorSource::Solid,
                /*_CustomColor         */ D2D1::ColorF(D2D1::ColorF::Black),
                /*_ColorIndex          */ 0,
                /*_ColorScheme         */ ColorScheme::Solid,
                /*_CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /*_Opacity             */ 1.f,
                /*_Thickness           */ 0.f,
                /*_FontName            */ L"",
                /*_FontSize            */ 0.f
            }
        },

        {
            VisualElement::GraphDescriptionText,
            {
                /* Name                */ L"Graph Description Text",
                /* UsedBy              */ VisualizationTypes::All,
                /*_Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                /*_ColorSource         */ ColorSource::Solid,
                /*_CustomColor         */ D2D1::ColorF(D2D1::ColorF::White),
                /*_ColorIndex          */ 0,
                /*_ColorScheme         */ ColorScheme::Solid,
                /*_CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /*_Opacity             */ 1.f,
                /*_Thickness           */ 0.f,
                /*_FontName            */ L"Segoe UI",
                /*_FontSize            */ 14.f,
            }
        },

        {
            VisualElement::GraphDescriptionBackground,
            {
                /* Name                */ L"Graph Description Background",
                /* UsedBy              */ VisualizationTypes::All,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(1.f, 1.f, 1.f, .25f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::XAxisText,
            {
                /* Name                */ L"X-axis Text",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectogram | VisualizationTypes::PeakMeter | VisualizationTypes::Oscilloscope,
                /*_Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                /*_ColorSource         */ ColorSource::Solid,
                /*_CustomColor         */ D2D1::ColorF(D2D1::ColorF::White),
                /*_ColorIndex          */ 0,
                /*_ColorScheme         */ ColorScheme::Solid,
                /*_CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /*_Opacity             */ 1.f,
                /*_Thickness           */ 0.f,
                /*_FontName            */ L"Segoe UI",
                /*_FontSize            */ 6.f
            }
        },

        {
            VisualElement::XAxisLine,
            {
                /* Name                */ L"X-axis Line",
                /* UsedBy              */ VisualizationTypes::Oscilloscope,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::YAxisText,
            {
                /* Name                */ L"Y-axis Text",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectogram | VisualizationTypes::PeakMeter | VisualizationTypes::Oscilloscope,
                /*_Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                /*_ColorSource         */ ColorSource::Solid,
                /*_CustomColor         */ D2D1::ColorF(D2D1::ColorF::White),
                /*_ColorIndex          */ 0,
                /*_ColorScheme         */ ColorScheme::Solid,
                /*_CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /*_Opacity             */ 1.f,
                /*_Thickness           */ 0.f,
                /*_FontName            */ L"Segoe UI",
                /*_FontSize            */ 6.f
            }
        },

        {
            VisualElement::YAxisLine,
            {
                /* Name                */ L"Y-axis Line",
                /* UsedBy              */ VisualizationTypes::Oscilloscope,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::HorizontalGridLine,
            {
                /* Name                */ L"Horizontal Grid Line",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::PeakMeter | VisualizationTypes::Spectogram | VisualizationTypes::Oscilloscope,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::VerticalGridLine,
            {
                /* Name                */ L"Vertical Grid Line",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectogram | VisualizationTypes::Oscilloscope,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::NyquistMarker,
            {
                /* Name                */ L"Nyquist Frequency Line",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::Curve | VisualizationTypes::Spectogram,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },
        #pragma endregion

        #pragma region Spectrum Bars, Radial Bars
        {
            VisualElement::BarArea,
            {
                /* Name                */ L"Bar Area",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::RadialBars,
                style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarTop,
            {
                /* Name                */ L"Bar Top",
                /* UsedBy              */ VisualizationTypes::Bars | VisualizationTypes::RadialBars,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::None, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 5.f, L"", 0.f,
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

        #pragma region Spectogram
        {
            VisualElement::Spectogram,
            {
                /* Name                */ L"Spectogram",
                /* UsedBy              */ VisualizationTypes::Spectogram,
                /*_Flags               */ style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware | style_t::Features::AmplitudeBasedColor | style_t::Features::HorizontalGradient,
                /*_ColorSource         */ ColorSource::Gradient,
                /*_CustomColor         */ D2D1::ColorF(D2D1::ColorF::Black),
                /*_ColorIndex          */ 0,
                /*_ColorScheme         */ ColorScheme::SoX,
                /*_CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Custom),
                /*_Opacity             */ 1.f,
                /*_Thickness           */ 0.f,
                /*_FontName            */ L"",
                /*_FontSize            */ 0.f
            }
        },
        #pragma endregion

        #pragma region Peak Meter
        {
            VisualElement::GaugeBackground,
            {
                /* Name                */ L"Gauge Background",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugePeakLevel,
            {
                /* Name                */ L"Peak Level",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Gauge0dBPeakLevel,
            {
                /* Name                */ L"Peak Level (> 0dB)",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMaxPeakLevel,
            {
                /* Name                */ L"Peak Level (Max)",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugePeakLevelText,
            {
                /* Name                */ L"Peak Level Read Out",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 10.f,
            }
        },

        // RMS
        {
            VisualElement::GaugeRMSLevel,
            {
                /* Name                */ L"RMS Level",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Gauge0dBRMSLevel,
            {
                /* Name                */ L"RMS Level (> 0dB)",
                /* UsedBy              */ VisualizationTypes::PeakMeter,
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeRMSLevelText,
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
            VisualElement::GaugeLeftRight,
            {
                /* Name                */ L"Left/Right Level",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeLeftRightIndicator,
            {
                /* Name                */ L"Left/Right Level Indicator",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMidSide,
            {
                /* Name                */ L"Mid/Side Level",
                /* UsedBy              */ VisualizationTypes::LevelMeter,
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMidSideIndicator,
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
                /*_Flags               */ style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                /*_ColorSource         */ ColorSource::Solid,
                /*_CustomColor         */ D2D1::ColorF(104.f/255.f, 208.f/255.f, 208.f/255.f, 1.f),
                /*_ColorIndex          */ 0,
                /*_ColorScheme         */ ColorScheme::Solid,
                /*_CustomGradientStops */ GetBuiltInGradientStops(ColorScheme::Solid),
                /*_Opacity             */ 1.f,
                /*_Thickness           */ 1.5f,
                /*_FontName            */ L"",
                /*_FontSize            */ 0.f
            }
        },
        #pragma endregion
    };

    const uint32_t _CurrentVersion = 5;
};


/** $VER: StyleManager.h (2024.02.10) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "framework.h"

#include "Style.h"
#include "Gradients.h"

#include <map>

class StyleManager
{
public:
    StyleManager();

    StyleManager(const StyleManager &) = delete;
    StyleManager & operator=(const StyleManager & other);
    StyleManager(StyleManager &&) = delete;
    StyleManager & operator=(StyleManager &&) = delete;

    virtual ~StyleManager() { }

    void Reset() noexcept;

    void Read(ui_element_config_parser & parser) noexcept;
    void Write(ui_element_config_builder & builder) const noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

    Style * GetStyle(VisualElement visualElement);

    void GetStyles(std::vector<Style> & styles) const;

    void SetArtworkDependentParameters(const GradientStops & gs, D2D1_COLOR_F dominantColor);

    void ReleaseDeviceSpecificResources();

private:
    std::map<VisualElement, Style> _Styles;

    #pragma warning(disable: 4868)
    const std::map<VisualElement, Style> _DefaultStyles
    {
    {
        VisualElement::Background,
        {
            "Background", Style::SupportsOpacity,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::XAxisLine,
        {
            "X-axis Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, "", 0.f,
        }
    },

    {
        VisualElement::XAxisText,
        {
            "X-axis Text", Style::SupportsOpacity | Style::SupportsFont,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::YAxisLine,
        {
            "Y-axis Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            "", 0.f,
        }
    },

    {
        VisualElement::YAxisText,
        {
            "Y-axis Text", Style::SupportsOpacity | Style::SupportsFont,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarSpectrum,
        {
            "Bar Spectrum", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarDarkBackground,
        {
            "Bar Dark Background", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarLightBackground,
        {
            "Bar Light Background", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarPeakIndicator,
        {
            "Bar Peak Indicator", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, "", 0.f,
        }
    },

    {
        VisualElement::CurveLine,
        {
            "Curve Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, "", 0.f,
        }
    },

    {
        VisualElement::CurveArea,
        {
            "Curve Area", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .5f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::CurvePeakLine,
        {
            "Curve Peak Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, "", 0.f,
        }
    },

    {
        VisualElement::CurvePeakArea,
        {
            "Curve Peak Area", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .25f, 0.f, "", 0.f,
        }
    }
    };

    const uint32_t _CurrentVersion = 2;
};

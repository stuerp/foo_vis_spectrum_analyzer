
/** $VER: StyleManager.h (2024.02.24) P. Stuer - Creates and manages the DirectX resources of the styles. **/

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

    Style * operator[] (size_t index)
    {
        std::map<VisualElement, Style>::iterator Iter(_Styles.begin());

        std::advance(Iter, index);

        return &Iter->second;
    }

    void Reset() noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

    Style * GetStyle(VisualElement visualElement);
    Style * GetStyleByIndex(int index);

    void SetArtworkDependentParameters(const GradientStops & gs, D2D1_COLOR_F dominantColor);

    void ResetGradients()
    {
        for (auto & Iter : _Styles)
        {
            if (Iter.second._ColorSource == ColorSource::Gradient)
                Iter.second._Brush = nullptr;
        }
    }

    void ReleaseDeviceSpecificResources();

private:
    std::map<VisualElement, Style> _Styles;

    #pragma warning(disable: 4868)
    const std::map<VisualElement, Style> _DefaultStyles
    {
    {
        VisualElement::GraphBackground,
        {
            Style::SupportsOpacity,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::GraphDescriptionText,
        {
            Style::SupportsOpacity | Style::SupportsFont,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::GraphDescriptionBackground,
        {
            Style::SupportsOpacity,
            ColorSource::Solid, D2D1::ColorF(1.f, 1.f, 1.f, .25f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::XAxisLine,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, "", 0.f,
        }
    },

    {
        VisualElement::XAxisText,
        {
            Style::SupportsOpacity | Style::SupportsFont,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::YAxisLine,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            "", 0.f,
        }
    },

    {
        VisualElement::YAxisText,
        {
            Style::SupportsOpacity | Style::SupportsFont,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarSpectrum,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarDarkBackground,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarLightBackground,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::BarPeakIndicator,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, "", 0.f,
        }
    },

    {
        VisualElement::CurveLine,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, "", 0.f,
        }
    },

    {
        VisualElement::CurveArea,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .5f, 0.f, "", 0.f,
        }
    },

    {
        VisualElement::CurvePeakLine,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, "", 0.f,
        }
    },

    {
        VisualElement::CurvePeakArea,
        {
            Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .25f, 0.f, "", 0.f,
        }
    }
    };

    const uint32_t _CurrentVersion = 4;
};

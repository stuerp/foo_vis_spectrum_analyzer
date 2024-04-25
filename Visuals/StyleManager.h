
/** $VER: StyleManager.h (2024.04.25) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "framework.h"

#include "Style.h"
#include "Gradients.h"

#include <map>

#pragma warning(disable: 4820)
class StyleManager
{
public:
    StyleManager();

    StyleManager(const StyleManager &) = delete;
    StyleManager & operator=(const StyleManager & other);
    StyleManager(StyleManager &&) = delete;
    StyleManager & operator=(StyleManager &&) = delete;

    virtual ~StyleManager() { }

    Style * operator[](size_t index)
    {
        std::map<VisualElement, Style>::iterator Iter(_Styles.begin());

        std::advance(Iter, index);

        return &Iter->second;
    }

    void Reset() noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

    /// <summary>
    /// Gets the style of the specified visual element.
    /// </summary>
    Style * GetStyle(VisualElement visualElement)
    {
        return &_Styles[visualElement];
    }

    /// <summary>
    /// Gets the default style of the specified visual element.
    /// </summary>
    Style * GetDefaultStyle(VisualElement visualElement)
    {
        return &_Styles[visualElement];
    }

    Style * GetStyleByIndex(int index);

    void SetArtworkDependentParameters(const GradientStops & gs, D2D1_COLOR_F dominantColor);

    void UpdateCurrentColors();

    void ReleaseGradientBrushes()
    {
        for (auto & Iter : _Styles)
        {
            if (Iter.second._ColorSource == ColorSource::Gradient)
                Iter.second._Brush.Release();
        }
    }

    // Helper
    HRESULT GetInitializedStyle(VisualElement visualElement, ID2D1RenderTarget * renderTarget, const D2D1_SIZE_F & size, const std::wstring & text, Style ** style) noexcept
    {
        if (*style != nullptr)
        {
            if ((*style)->_Brush != nullptr)
                return S_OK;
            else
                return (*style)->CreateDeviceSpecificResources(renderTarget, text, size);
        }

        *style = GetStyle(visualElement);

        if (*style == nullptr)
            return E_FAIL;

        if ((*style)->_Brush != nullptr)
            return S_OK;

        return (*style)->CreateDeviceSpecificResources(renderTarget, text, size);
    }

    void ReleaseDeviceSpecificResources();

public:
    std::vector<D2D1_COLOR_F> _UserInterfaceColors;
    D2D1_COLOR_F _DominantColor;                                        // The current dominant color extracted from the artwork bitmap.

private:
    std::map<VisualElement, Style> _Styles;

    #pragma warning(disable: 4868)
    std::map<VisualElement, Style> _DefaultStyles
    {
        {
            VisualElement::GraphBackground,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GraphDescriptionText,
            {
                Style::SupportsOpacity | Style::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 14.f,
            }
        },

        {
            VisualElement::GraphDescriptionBackground,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(1.f, 1.f, 1.f, .25f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::VerticalGridLine,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::XAxisText,
            {
                Style::SupportsOpacity | Style::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 6.f,
            }
        },

        {
            VisualElement::HorizontalGridLine,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::YAxisText,
            {
                Style::SupportsOpacity | Style::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 6.f,
            }
        },

        {
            VisualElement::BarArea,
            {
                Style::SupportsOpacity | Style::AmplitudeAware,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarTop,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::None, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 5.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarPeakArea,
            {
                Style::SupportsOpacity | Style::AmplitudeAware,
                ColorSource::None, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 0.25f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarPeakTop,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarDarkBackground,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarLightBackground,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurveLine,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurveArea,
            {
                Style::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .5f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurvePeakLine,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurvePeakArea,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .25f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Spectogram,
            {
                Style::SupportsOpacity | Style::AmplitudeAware | Style::AmplitudeBasedColor,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::SoX, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        // Peak
        {
            VisualElement::GaugeBackground,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugePeakLevel,
            {
                Style::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Gauge0dBPeakLevel,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMaxPeakLevel,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugePeakLevelText,
            {
                Style::SupportsOpacity | Style::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 10.f,
            }
        },

        // RMS
        {
            VisualElement::GaugeRMSLevel,
            {
                Style::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Gauge0dBRMSLevel,
            {
                Style::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeRMSLevelText,
            {
                Style::SupportsOpacity | Style::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 10.f,
            }
        },

        {
            VisualElement::NyquistMarker,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        // Level Meter
        {
            VisualElement::GaugeLeftRight,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMidSide,
            {
                Style::SupportsOpacity | Style::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::LevelMeterAxis,
            {
                Style::SupportsOpacity | Style::SupportsThickness | Style::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 0.5f, 1.f, L"Segoe UI", 10.f,
            }
        },
    };

    const uint32_t _CurrentVersion = 5;
};

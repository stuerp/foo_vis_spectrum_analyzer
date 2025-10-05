
/** $VER: StyleManager.h (2025.10.05) P. Stuer - Creates and manages the DirectX resources of the styles. **/

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

    style_t * operator[](size_t index)
    {
        std::map<VisualElement, style_t>::iterator Iter(_Styles.begin());

        std::advance(Iter, index);

        return &Iter->second;
    }

    void Reset() noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

    /// <summary>
    /// Gets the style of the specified visual element.
    /// </summary>
    style_t * GetStyle(VisualElement visualElement) noexcept
    {
        return &_Styles[visualElement];
    }

    /// <summary>
    /// Gets the default style of the specified visual element.
    /// </summary>
    style_t * GetDefaultStyle(VisualElement visualElement) noexcept
    {
        return &_Styles[visualElement];
    }

    style_t * GetStyleByIndex(int index) noexcept;

    void SetArtworkDependentParameters(const gradient_stops_t & gs, D2D1_COLOR_F dominantColor) noexcept;

    void UpdateCurrentColors() noexcept;

    void ReleaseGradientBrushes() noexcept
    {
        for (auto & Iter : _Styles)
        {
            if (Iter.second._ColorSource == ColorSource::Gradient)
                Iter.second._Brush.Release();
        }
    }

    // Helper
    HRESULT GetInitializedStyle(VisualElement visualElement, ID2D1RenderTarget * renderTarget, const D2D1_SIZE_F & size, const std::wstring & text, style_t ** style) noexcept
    {
        if (*style == nullptr)
        {
            *style = GetStyle(visualElement);

            if (*style == nullptr)
                return E_FAIL;
        }

        if ((*style)->_Brush != nullptr)
            return S_OK;

        return (*style)->CreateDeviceSpecificResources(renderTarget, size, text);
    }

    // Helper for radial bars
    HRESULT GetInitializedStyle(VisualElement visualElement, ID2D1RenderTarget * renderTarget, const D2D1_SIZE_F & size, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, style_t ** style) noexcept
    {
        if (*style == nullptr)
        {
            *style = GetStyle(visualElement);

            if (*style == nullptr)
                return E_FAIL;
        }

        if ((*style)->_Brush != nullptr)
            return S_OK;

        return (*style)->CreateDeviceSpecificResources(renderTarget, size, center, offset, rx, ry, rOffset);
    }

    void ReleaseDeviceSpecificResources() noexcept;

public:
    std::vector<D2D1_COLOR_F> _UserInterfaceColors;
    D2D1_COLOR_F _DominantColor;                                        // The current dominant color extracted from the artwork bitmap.

private:
    std::map<VisualElement, style_t> _Styles;

    #pragma warning(disable: 4868)
    std::map<VisualElement, style_t> _DefaultStyles
    {
        {
            VisualElement::GraphBackground,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GraphDescriptionText,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 14.f,
            }
        },

        {
            VisualElement::GraphDescriptionBackground,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(1.f, 1.f, 1.f, .25f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::VerticalGridLine,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::XAxisText,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 6.f,
            }
        },

        {
            VisualElement::HorizontalGridLine,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::YAxisText,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 6.f,
            }
        },

        {
            VisualElement::BarArea,
            {
                style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarTop,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::None, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 5.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarPeakArea,
            {
                style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware,
                ColorSource::None, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 0.25f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarPeakTop,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarDarkBackground,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::BarLightBackground,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurveLine,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 2.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurveArea,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), .5f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurvePeakLine,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 2.f, L"", 0.f,
            }
        },

        {
            VisualElement::CurvePeakArea,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), .25f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Spectogram,
            {
                style_t::Features::SupportsOpacity | style_t::Features::AmplitudeAware | style_t::Features::AmplitudeBasedColor,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::SoX, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        // Peak
        {
            VisualElement::GaugeBackground,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugePeakLevel,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Gauge0dBPeakLevel,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMaxPeakLevel,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugePeakLevelText,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 10.f,
            }
        },

        // RMS
        {
            VisualElement::GaugeRMSLevel,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::Gauge0dBRMSLevel,
            {
                style_t::Features::SupportsOpacity,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeRMSLevelText,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 0.f, L"Segoe UI", 10.f,
            }
        },

        {
            VisualElement::NyquistMarker,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Red), 0, ColorScheme::Artwork, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        // Level Meter
        {
            VisualElement::GaugeLeftRight,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeLeftRightIndicator,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMidSide,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::GaugeMidSideIndicator,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 1.f, 1.f, L"", 0.f,
            }
        },

        {
            VisualElement::LevelMeterAxis,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness | style_t::Features::SupportsFont,
                ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Prism1, GetBuiltInGradientStops(ColorScheme::Custom), 0.5f, 1.f, L"Segoe UI", 10.f,
            }
        },

        // Oscilloscope
        {
            VisualElement::SignalLine,
            {
                style_t::Features::SupportsOpacity | style_t::Features::SupportsThickness,
                ColorSource::Solid, D2D1::ColorF(104.f/255.f, 208.f/255.f, 208.f/255.f, 1.f), 0, ColorScheme::Solid, GetBuiltInGradientStops(ColorScheme::Solid), 1.f, 1.5f, L"", 0.f,
            }
        },
    };

    const uint32_t _CurrentVersion = 5;
};


/** $VER: StyleManager.cpp (2024.02.04) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "StyleManager.h"

#include "Gradients.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
StyleManager::StyleManager()
{
    Reset();
}

/// <summary>
/// Implements the = operator.
/// </summary>
StyleManager & StyleManager::operator=(const StyleManager & other)
{
    for (const auto & Iter : other._Styles)
        _Styles[Iter.first] = Iter.second;

    return *this;
}

/// <summary>
/// Resets this instance.
/// </summary>
void StyleManager::Reset() noexcept
{
    _Styles.clear();

    _Styles.insert
    ({
        VisualElement::Background,
        {
            L"Background", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::XAxisLine,
        {
            L"X-axis Line", ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::XAxisText,
        {
            L"X-axis Text", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::YAxisLine,
        {
            L"Y-axis Line", ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::YAxisText,
        {
            L"Y-axis Text", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarForeground,
        {
            L"Bar Foreground", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarDarkBackground,
        {
            L"Bar Dark Background", ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarLightBackground,
        {
            L"Bar Light Background", ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarPeakIndicator,
        {
            L"Bar Peak Indicator", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurveLine,
        {
            L"Curve Line", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurveArea,
        {
            L"Curve Area", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .5f, 0.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurvePeakLine,
        {
            L"Curve Peak Line", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f,
            L"", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurvePeakArea,
        {
            L"Curve Peak Area", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .25f, 0.f,
            L"", 0.f,
        }
    });

    for (auto & Iter : _Styles)
    {
        Iter.second._Color = Iter.second._CustomColor;
        Iter.second._GradientStops = GetGradientStops(Iter.second._ColorScheme);
    }
}

/// <summary>
/// Gets the style of the specified visual element.
/// </summary>
Style * StyleManager::GetStyle(VisualElement visualElement)
{
    return &_Styles[visualElement];
}

/// <summary>
/// Gets a list of all the styles.
/// </summary>
void StyleManager::GetStyles(std::vector<Style> & styles) const
{
    for (const auto & Iter : _Styles)
        styles.push_back(Iter.second);
}

/// <summary>
/// Updates the style parameters of all styles that are using the artwork as source.
/// </summary>
void StyleManager::SetArtworkDependentParameters(const GradientStops & gs, D2D1_COLOR_F dominantColor)
{
    for (auto & Iter : _Styles)
    {
        if (Iter.second._ColorSource == ColorSource::Gradient)
        {
            if (Iter.second._ColorScheme == ColorScheme::Artwork)
            {
                Iter.second._GradientStops = gs;
                Iter.second._Brush.Release();
            }
        }
        else
        if (Iter.second._ColorSource == ColorSource::DominantColor)
        {
            Iter.second._Color = dominantColor;
            Iter.second._Brush.Release();
        }
    }
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void StyleManager::ReleaseDeviceSpecificResources()
{
    for (auto & Iter : _Styles)
        Iter.second._Brush.Release();
}

StyleManager _StyleManager;

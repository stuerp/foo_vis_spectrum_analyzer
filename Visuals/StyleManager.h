
/** $VER: StyleManager.h (2024.01.31) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "framework.h"

#include "Style.h"

#include <map>

class StyleManager
{
public:
    Style & GetStyle(VisualElement visualElement);

    void GetStyles(std::vector<Style> & styles) const;

    void ReleaseDeviceSpecificResources();

private:
    HRESULT CreateGradientBrush(ID2D1RenderTarget * renderTarget, FLOAT width, FLOAT height, const GradientStops & gradientStops, bool isVertical, ID2D1LinearGradientBrush ** brush);

private:
    std::map<VisualElement, Style> _Styles = 
    {
        { VisualElement::Background,
            {
                L"Background", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::XAxisLine,
            {
                L"X-axis Line", ColorSource::Gradient, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                1.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::XAxisText,
            {
                L"X-axis Text", ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::YAxisLine,
            {
                L"Y-axis Line", ColorSource::Gradient, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                1.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::YAxisText,
            {
                L"Y-axis Text", ColorSource::Gradient, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::BarForeground,
            {
                L"Bar Foreground", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, { }, 1.f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::BarDarkBackground,
            {
                L"Bar Dark Background", ColorSource::Solid, D2D1::ColorF(0), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::BarLightBackground,
            {
                L"Bar Light Background", ColorSource::Solid, D2D1::ColorF(0), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::BarPeakIndicator,
            {
                L"Bar Peak Indicator", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, { }, 1.f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::CurveLine,
            {
                L"Curve Line", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, { }, 1.f,
                // Line-specific
                2.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::CurveArea,
            {
                L"Curve Area", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, { }, .5f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::CurvePeakLine,
            {
                L"Peak Line", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, { }, 1.f,
                // Line-specific
                2.f,
                // Font-specific
                L"", 0.f,
            }
        },

        { VisualElement::CurvePeakArea,
            {
                L"Peak Area", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, { }, .25f,
                // Line-specific
                0.f,
                // Font-specific
                L"", 0.f,
            }
        },
    };
};

extern StyleManager _StyleManager;

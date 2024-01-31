
/** $VER: StyleManager.cpp (2024.01.31) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "StyleManager.h"

#include <map>

#pragma hdrstop

std::map<VisualElement, Style> _Styles = 
{
    { VisualElement::Background,
        {
            L"Background", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), ColorScheme::Solid, { },
            // Area-specific
            1.f,
            // Line-specific
            0.f,
            // Font-specific
            L"", 0.f,
        }
    },

    { VisualElement::CurveLine,
        {
            L"Curve Line", ColorSource::Gradient, D2D1::ColorF(0), ColorScheme::Artwork, { },
            // Area-specific
            1.f,
            // Line-specific
            2.f,
            // Font-specific
            L"", 0.f,
        }
    },

    { VisualElement::CurveArea,
        {
            L"Curve Area", ColorSource::Gradient, D2D1::ColorF(0), ColorScheme::Artwork, { },
            // Area-specific
            .5f,
            // Line-specific
            0.f,
            // Font-specific
            L"", 0.f,
        }
    },

    { VisualElement::PeakLine,
        {
            L"Peak Line", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), ColorScheme::Artwork, { },
            // Area-specific
            1.f,
            // Line-specific
            2.f,
            // Font-specific
            L"", 0.f,
        }
    },

    { VisualElement::PeakArea,
        {
            L"Peak Area", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), ColorScheme::Artwork, { },
            // Area-specific
            .5f,
            // Line-specific
            0.f,
            // Font-specific
            L"", 0.f,
        }
    },
};

/// <summary>
/// Gets the style of the specified visual element.
/// </summary>
Style & StyleManager::GetStyle(VisualElement visualElement)
{
    Style & style = _Styles[VisualElement::Background];

    return style;
}

/// <summary>
/// Creates a gradient brush.
/// </summary>
HRESULT StyleManager::CreateGradientBrush(ID2D1RenderTarget * renderTarget, FLOAT width, FLOAT height, const GradientStops & gradientStops, bool isVertical, ID2D1LinearGradientBrush ** brush)
{
    if (gradientStops.empty())
        return E_FAIL;

    CComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = renderTarget->CreateGradientStopCollection(&gradientStops[0], (UINT32) gradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F Size = renderTarget->GetSize();

        D2D1_POINT_2F Start = isVertical ? D2D1::Point2F(Size.width / 2.f, 0.f)         : D2D1::Point2F(       0.f, Size.height / 2.f);
        D2D1_POINT_2F End   = isVertical ? D2D1::Point2F(Size.width / 2.f, Size.height) : D2D1::Point2F(Size.width, Size.height / 2.f);

        hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(Start, End), Collection, brush);
    }

    return hr;
}

StyleManager _StyleManager;

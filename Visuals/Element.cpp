
/** $VER: Element.cpp (2024.02.03) P. Stuer - Base class for all visual elements **/

#include "Element.h"

#pragma hdrstop

/// <summary>
/// Initializes the DirectX resources of a style->
/// </summary>
HRESULT Element::InitializeStyle(ID2D1RenderTarget * renderTarget, Style * style) noexcept
{
    HRESULT hr = S_OK;

    if (style->_ColorSource != ColorSource::Gradient)
        hr = renderTarget->CreateSolidColorBrush(style->_Color, (ID2D1SolidColorBrush ** ) &style->_Brush);
    else
    {
        ID2D1LinearGradientBrush * Brush;

        hr = CreateGradientBrush(renderTarget, style->_GradientStops, &Brush);

        if (SUCCEEDED(hr))
            style->_Brush.Attach(Brush);
    }

    if (style->_Brush)
        style->_Brush->SetOpacity(style->_Opacity);

    return hr;
}

/// <summary>
/// Creates a gradient brush for rendering the bars.
/// </summary>
HRESULT Element::CreateGradientBrush(ID2D1RenderTarget * renderTarget, const GradientStops & gradientStops, ID2D1LinearGradientBrush ** gradientBrush)
{
    if (gradientStops.empty())
        return E_FAIL;

    CComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = renderTarget->CreateGradientStopCollection(&gradientStops[0], (UINT32) gradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F Size = renderTarget->GetSize();

        D2D1_POINT_2F Start = _Configuration->_HorizontalGradient ? D2D1::Point2F(       0.f, Size.height / 2.f) : D2D1::Point2F(Size.width / 2.f, 0.f);
        D2D1_POINT_2F End   = _Configuration->_HorizontalGradient ? D2D1::Point2F(Size.width, Size.height / 2.f) : D2D1::Point2F(Size.width / 2.f, Size.height);

        hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(Start, End), Collection, gradientBrush);
    }

    return hr;
}

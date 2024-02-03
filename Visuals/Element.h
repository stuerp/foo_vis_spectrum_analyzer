
/** $VER: Element.h (2024.02.03) P. Stuer - Base class for all visual elements. **/

#pragma once

#include "framework.h"
#include "Support.h"

#include "Configuration.h"
#include "Style.h"

class Element
{
public:
    Element() : _Configuration() {}

protected:
    HRESULT InitializeStyle(ID2D1RenderTarget * renderTarget, Style * style) noexcept;
    HRESULT CreateGradientBrush(ID2D1RenderTarget * renderTarget, const GradientStops & gradientStops, ID2D1LinearGradientBrush ** gradientBrush);

protected:
    const Configuration * _Configuration;
};

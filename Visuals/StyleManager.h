
/** $VER: StyleManager.h (2024.01.31) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "framework.h"

#include "Style.h"

class StyleManager
{
public:
    Style & GetStyle(VisualElement visualElement);

private:
    HRESULT CreateGradientBrush(ID2D1RenderTarget * renderTarget, FLOAT width, FLOAT height, const GradientStops & gradientStops, bool isVertical, ID2D1LinearGradientBrush ** brush);
};

extern StyleManager _StyleManager;

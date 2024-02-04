
/** $VER: StyleManager.h (2024.02.04) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "framework.h"

#include "Style.h"

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

    Style * GetStyle(VisualElement visualElement);

    void GetStyles(std::vector<Style> & styles) const;

    void SetArtworkDependentParameters(const GradientStops & gs, D2D1_COLOR_F dominantColor);

    void ReleaseDeviceSpecificResources();

private:
    std::map<VisualElement, Style> _Styles;
};

extern StyleManager _StyleManager;

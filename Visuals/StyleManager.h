
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

    void Read(ui_element_config_parser & parser) noexcept;
    void Write(ui_element_config_builder & builder) const noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler) const noexcept;

    Style * GetStyle(VisualElement visualElement);

    void GetStyles(std::vector<Style> & styles) const;

    void SetArtworkDependentParameters(const GradientStops & gs, D2D1_COLOR_F dominantColor);

    void ReleaseDeviceSpecificResources();

private:
    std::map<VisualElement, Style> _Styles;

    const size_t _CurrentVersion = 1;
};

extern StyleManager _StyleManager;

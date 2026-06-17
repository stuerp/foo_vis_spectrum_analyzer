
/** $VER: StyleManager.h (2026.06.17) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#pragma once

#include "pch.h"

#include "Style.h"
#include "Gradients.h"

#pragma warning(disable: 4868) // compiler may not enforce left-to-right evaluation order in braced initializer list

#include <nlohmann\json.hpp>

using json = nlohmann::ordered_json;

#pragma warning(default: 4868)

class style_manager_t
{
public:
    style_manager_t();

    style_manager_t(const style_manager_t & other);
    style_manager_t & operator=(const style_manager_t & other);

    style_manager_t(const style_manager_t && other) = delete;
    style_manager_t & operator=(const style_manager_t && other) = delete;

    virtual ~style_manager_t() noexcept { }

    void Reset() noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

    json ToJSON() const noexcept;
    void FromJSON(const json & array) noexcept;

    /// <summary>
    /// Gets a copy of the style of the specified visual element.
    /// </summary>
    void GetStyle(VisualElement visualElement, style_t & style) noexcept
    {
        style = _Styles[visualElement];
    }

    /// <summary>
    /// Gets the style of the specified visual element.
    /// </summary>
    style_t * GetStyle(VisualElement visualElement) noexcept
    {
        return &_Styles[visualElement];
    }

    HRESULT GetInitializedStyle(VisualElement visualElement, ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const std::wstring & text, FLOAT scaleFactor, style_t & style) noexcept;
    HRESULT GetInitializedStyle(VisualElement visualElement, ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, style_t & style) noexcept;

private:
    static json ToJSON(const gradient_stops_t & gradientStops) noexcept;
    static gradient_stops_t FromJSONGradientStops(const json::array_t & array) noexcept;

    static json ToJSON(const D2D1_GRADIENT_STOP & gs) noexcept;
    static D2D1_GRADIENT_STOP FromJSONGradientStop(const json & object) noexcept;

    static json ToJSON(const D2D1_COLOR_F & color) noexcept;
    static D2D1_COLOR_F FromJSONColor(const json & object) noexcept;

private:
    std::unordered_map<VisualElement, style_t> _Styles;

    static std::unordered_map<VisualElement, style_t> _DefaultStyles;

    const uint32_t _CurrentVersion = 5;
};

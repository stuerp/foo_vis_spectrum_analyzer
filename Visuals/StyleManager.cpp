
/** $VER: StyleManager.cpp (2026.03.21) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "pch.h"

#include "StyleManager.h"

#include "Resources.h"
#include "Log.h"

#include <exception>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
style_manager_t::style_manager_t()
{
    Reset();
}

/// <summary>
/// Implements the = operator.
/// </summary>
style_manager_t & style_manager_t::operator=(const style_manager_t & other)
{
    for (const auto & [ID, Style] : other.Styles)
        Styles[ID] = Style;

    return *this;
}

/// <summary>
/// Resets this instance.
/// </summary>
void style_manager_t::Reset() noexcept
{
    Styles = _DefaultStyles;

    for (auto & [ID, Style] : Styles)
    {
        Style._CurrentColor         = Style._CustomColor;
        Style._CurrentGradientStops = GetBuiltInGradientStops(Style._ColorScheme);
    }
}

/// <summary>
/// Updates the style parameters of all styles that are using the artwork as source.
/// </summary>
void style_manager_t::SetArtworkDependentParameters(const gradient_stops_t & gs, D2D1_COLOR_F dominantColor) noexcept
{
    for (auto & [ID, Style] : Styles)
    {
        if (Style._ColorSource == ColorSource::Gradient)
        {
            if (Style._ColorScheme == ColorScheme::Artwork)
            {
                Style._CurrentGradientStops = gs;
                Style.DeleteDeviceSpecificResources();
            }
        }
        else
        if (Style._ColorSource == ColorSource::DominantColor)
        {
            Style._CurrentColor = dominantColor;
            Style.DeleteDeviceSpecificResources();
        }
    }
}

/// <summary>
/// Updates the current color of each style.
/// </summary>
void style_manager_t::UpdateCurrentColors() noexcept
{
    for (auto & [ID, Style] : Styles)
        Style.UpdateCurrentColor(DominantColor, UserInterfaceColors);
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void style_manager_t::DeleteDeviceSpecificResources() noexcept
{
    for (auto & [ID, Style] : Styles)
        Style.DeleteDeviceSpecificResources();
}

/// <summary>
/// Gets the style with initialized DirectX resources.
/// </summary>
HRESULT style_manager_t::GetInitializedStyle(VisualElement visualElement, ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const std::wstring & text, FLOAT scaleFactor, style_t ** style) noexcept
{
    if (*style == nullptr)
    {
        *style = GetStyle(visualElement);

        if (*style == nullptr)
            return E_FAIL;
    }

    if ((*style)->_Brush != nullptr)
        return S_OK;

    return (*style)->CreateDeviceSpecificResources(deviceContext, size, text, scaleFactor);
}

/// <summary>
/// Gets the style with initialized DirectX resources.
/// </summary>
HRESULT style_manager_t::GetInitializedStyle(VisualElement visualElement, ID2D1DeviceContext * deviceContext, const D2D1_SIZE_F & size, const D2D1_POINT_2F & center, const D2D1_POINT_2F & offset, FLOAT rx, FLOAT ry, FLOAT rOffset, style_t ** style) noexcept
{
    if (*style == nullptr)
    {
        *style = GetStyle(visualElement);

        if (*style == nullptr)
            return E_FAIL;
    }

    if ((*style)->_Brush != nullptr)
        return S_OK;

    return (*style)->CreateDeviceSpecificResources(deviceContext, size, center, offset, rx, ry, rOffset);
}

/// <summary>
/// Reads this instance from a stream.
/// </summary>
void style_manager_t::Read(stream_reader * reader, size_t size, abort_callback & abortHandler) noexcept
{
    try
    {
        uint32_t Version; reader->read_object_t(Version, abortHandler);

        if (Version > _CurrentVersion)
            return;

        size_t StyleCount; reader->read_object_t(StyleCount, abortHandler);

        for (size_t i = 0; i < StyleCount; ++i)
        {
            uint32_t Id; reader->read_object_t(Id, abortHandler);

            if ((Version < 3) && (Id != 0)) // Version 2: VisualElement::GraphDescription was added before VisualElement::XAxisLine
                Id++;

            if (Version < 4)
                pfc::string Name = reader->read_string(abortHandler);

            style_t Style = { };

            // Handle unknown styles. This can happen when an older component version reads a preset from a newer one.
            if (Id < (uint32_t) VisualElement::Count)
                Style = Styles[(VisualElement) Id];    

            uint64_t Flags;

            reader->read_object_t(Flags, abortHandler);

            // Add only the non-system flags to the style from the read value.
            Style._Flags = (Style._Flags & style_t::Features::System) | ((style_t::Features) Flags & ~style_t::Features::System); 

            uint32_t Integer;

            reader->read_object_t(Integer, abortHandler); Style._ColorSource = (ColorSource) Integer;
            reader->read_object(&Style._CustomColor, sizeof(Style._CustomColor), abortHandler);
            reader->read_object_t(Style._ColorIndex, abortHandler);
            reader->read_object_t(Integer, abortHandler); Style._ColorScheme = (ColorScheme) Integer;

            gradient_stops_t gs;

            size_t GradientStopCount; reader->read_object_t(GradientStopCount, abortHandler);

            for (size_t j = 0; j < GradientStopCount; ++j)
            {
                FLOAT Position; reader->read_object_t(Position, abortHandler);
                D2D1_COLOR_F Color; reader->read_object(&Color, sizeof(Color), abortHandler);

                gs.push_back({ Position, Color });
            }

            Style._CustomGradientStops = gs;

            reader->read_object_t(Style._Opacity, abortHandler);
            reader->read_object_t(Style._Thickness, abortHandler);

            pfc::string FontName = reader->read_string(abortHandler);
            FLOAT FontSize; reader->read_object_t(FontSize, abortHandler);

            if (Version > 4)
            {
                Style._FontName = pfc::wideFromUTF8(FontName);
                Style._FontSize = FontSize;
            }

            // Sets the default font settings.
            if (Style.Has(style_t::Features::SupportsFont))
            {
                const auto & DefaultStyle = _DefaultStyles[(VisualElement) Id];

                if (Style._FontName.empty())
                    Style._FontName = DefaultStyle._FontName;

                if (Style._FontSize < 2.f)
                    Style._FontSize = DefaultStyle._FontSize;
            }

            // 'Activate' the values we just read.
            if (Style._ColorScheme == ColorScheme::Custom)
                Style._CurrentGradientStops = Style._CustomGradientStops;
            else
                Style._CurrentGradientStops = GetBuiltInGradientStops(Style._ColorScheme);

            Style.UpdateCurrentColor(DominantColor, UserInterfaceColors);

            if (Id < (uint32_t) VisualElement::Count)
                Styles[(VisualElement) Id] = Style;
        }
    }
    catch (std::exception & ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to read styles: %s", ex.what());

        Reset();
    }
}

/// <summary>
/// Writes this instance to a stream.
/// </summary>
void style_manager_t::Write(stream_writer * writer, abort_callback & abortHandler) const noexcept
{
    try
    {
        writer->write_object_t(_CurrentVersion, abortHandler);

        size_t Size = Styles.size();

        writer->write_object_t(Size, abortHandler);

        for (const auto & Iter : Styles)
        {
            {
                uint32_t Id = (uint32_t) Iter.first;

                writer->write_object_t(Id, abortHandler);
            }

            {
                const style_t & Style = Iter.second;

                writer->write_object_t((uint64_t) Style._Flags, abortHandler);
                writer->write_object(&Style._ColorSource, sizeof(Style._ColorSource), abortHandler);
                writer->write_object(&Style._CustomColor, sizeof(Style._CustomColor), abortHandler);
                writer->write_object_t(Style._ColorIndex, abortHandler);
                writer->write_object(&Style._ColorScheme, sizeof(Style._ColorScheme), abortHandler);

                Size = Style._CustomGradientStops.size();

                writer->write_object_t(Size, abortHandler);

                for (const auto & gs : Style._CustomGradientStops)
                {
                    writer->write_object_t(gs.position, abortHandler);
                    writer->write_object(&gs.color, sizeof(gs.color), abortHandler);
                }

                writer->write_object_t(Style._Opacity, abortHandler);
                writer->write_object_t(Style._Thickness, abortHandler);

                pfc::string FontName = pfc::utf8FromWide(Style._FontName.c_str());
                writer->write_string(FontName, abortHandler);
                writer->write_object_t(Style._FontSize, abortHandler);
            }
        }
    }
    catch (std::exception & ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to write styles: %s", ex.what());
    }
}

/// <summary>
/// Serialize this instance to JSON.
/// </summary>
json style_manager_t::ToJSON() const noexcept
{
    json::array_t Array;

    try
    {
        for (const auto & Iter : Styles)
        {
            const style_t & Style = Iter.second;

            Array.push_back
            (
                json::object_t
                ({
                    { "id", (uint32_t) Iter.first },

                    { "flags", Style._Flags },
                    { "colorSource", Style._ColorSource },
                    { "customColor", style_manager_t::ToJSON(Style._CustomColor) },
                    { "colorIndex", Style._ColorIndex },
                    { "colorScheme", Style._ColorScheme },
                    { "customGradientStops", style_manager_t::ToJSON(Style._CustomGradientStops) },
                    { "opacity", Style._Opacity },
                    { "thickness", Style._Thickness },
                    { "font", json::object_t
                        ({
                            { "name", msc::WideToUTF8(Style._FontName) },
                            { "size", Style._FontSize },
                        })
                    },
                })
            );
        }
    }
    catch (std::exception & ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to serialize styles: %s", ex.what());

        Array.clear();
    }

    return Array;
}

/// <summary>
/// Deserializes this instance from JSON.
/// </summary>
void style_manager_t::FromJSON(const json & array) noexcept
{
    try
    {
        for (const auto & Iter : array)
        {
            const uint32_t Id = Iter.value("id", 0u);

            style_t Style = { };

            // Handle unknown styles. This can happen when an older component version reads a preset from a newer one.
            if (Id < (uint32_t) VisualElement::Count)
                Style = Styles[(VisualElement) Id];    

            Style._Flags = Iter.value("flags", Style._Flags);
            Style._ColorSource = Iter.value("colorSource", ColorSource::None);

            const auto & Color = Iter.value("customColor", json::object());

            Style._CustomColor = style_manager_t::FromJSONColor(Color);

            Style._ColorIndex = Iter.value("colorIndex", Style._ColorIndex);
            Style._ColorScheme = Iter.value("colorScheme", Style._ColorScheme);

            const auto & Array = Iter.value("customGradientStops", json::array());

            Style._CustomGradientStops = style_manager_t::FromJSONGradientStops(Array);

            Style._Opacity = Iter.value("opacity", Style._Opacity);
            Style._Thickness = Iter.value("thickness", Style._Thickness);

            const auto & Font = Iter.value("font", json::object());

            Style._FontName = msc::UTF8ToWide(Font.value("name", msc::WideToUTF8(Style._FontName)));
            Style._FontSize = Font.value("size", Style._FontSize);

            // Sets the default font settings.
            if (Style.Has(style_t::Features::SupportsFont))
            {
                const auto & DefaultStyle = _DefaultStyles[(VisualElement) Id];

                if (Style._FontName.empty())
                    Style._FontName = DefaultStyle._FontName;

                if (Style._FontSize < 2.f)
                    Style._FontSize = DefaultStyle._FontSize;
            }

            // 'Activate' the values we just read.
            if (Style._ColorScheme == ColorScheme::Custom)
                Style._CurrentGradientStops = Style._CustomGradientStops;
            else
                Style._CurrentGradientStops = GetBuiltInGradientStops(Style._ColorScheme);

            Style.UpdateCurrentColor(DominantColor, UserInterfaceColors);

            if (Id < (uint32_t) VisualElement::Count)
                Styles[(VisualElement) Id] = Style;
        }
    }
    catch (std::exception & ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to deserialize styles: %s", ex.what());
    }
}

/// <summary>
/// Serializes a gradient_stops_t type to JSON.
/// </summary>
json style_manager_t::ToJSON(const gradient_stops_t & gradientStops) noexcept
{
    json::array_t Array;

    for (const auto & gs : gradientStops)
        Array.push_back(ToJSON(gs));

    return Array;
}

/// <summary>
/// Deserializes a gradient_stops_t type from JSON.
/// </summary>
gradient_stops_t style_manager_t::FromJSONGradientStops(const json::array_t & array) noexcept
{
    gradient_stops_t gradientStops;

    for (const auto & Iter : array)
        gradientStops.push_back(FromJSONGradientStop(Iter));

    return gradientStops;
}

/// <summary>
/// Serializes a D2D1_GRADIENT_STOP structure to JSON.
/// </summary>
json style_manager_t::ToJSON(const D2D1_GRADIENT_STOP & gs) noexcept
{
    return json::object_t
    ({
        { "position", gs.position },
        { "color", ToJSON(gs.color) },
    });
}

/// <summary>
/// Deserializes a D2D1_GRADIENT_STOP type from JSON.
/// </summary>
D2D1_GRADIENT_STOP style_manager_t::FromJSONGradientStop(const json & object) noexcept
{
    D2D1_GRADIENT_STOP gs;

    gs.position = object.value("position", 0.f);
    gs.color    = FromJSONColor(object.value("color", json::object()));

    return gs;
}   

/// <summary>
/// Serializes a D2D1_COLOR_F structure to JSON.
/// </summary>
json style_manager_t::ToJSON(const D2D1_COLOR_F & color) noexcept
{
    return json::object_t
    ({
        { "red", color.r },
        { "green", color.g },
        { "blue", color.b },
        { "alpha", color.a },
    });
}

/// <summary>
/// Deserializes a D2D1_COLOR_F structure from JSON.
/// </summary>
D2D1_COLOR_F style_manager_t::FromJSONColor(const json & object) noexcept
{
    return D2D1::ColorF(object.value("red", 0.f), object.value("green", 0.f), object.value("blue", 0.f), object.value("alpha", 1.f));
}

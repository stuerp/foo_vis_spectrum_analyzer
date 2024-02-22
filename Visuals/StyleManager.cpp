
/** $VER: StyleManager.cpp (2024.02.18) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "StyleManager.h"

#include "Log.h"

#include <exception>

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
    _Styles = _DefaultStyles;

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
                Iter.second.ReleaseDeviceSpecificResources();
            }
        }
        else
        if (Iter.second._ColorSource == ColorSource::DominantColor)
        {
            Iter.second._Color = dominantColor;
            Iter.second.ReleaseDeviceSpecificResources();
        }
    }
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void StyleManager::ReleaseDeviceSpecificResources()
{
    for (auto & Iter : _Styles)
        Iter.second.ReleaseDeviceSpecificResources();
}

/// <summary>
/// Reads this instance from a stream.
/// </summary>
void StyleManager::Read(stream_reader * reader, size_t size, abort_callback & abortHandler) noexcept
{
    try
    {
        uint32_t Version; reader->read_object_t(Version, abortHandler);

        if (Version > _CurrentVersion)
            return;

        _Styles.clear();

        size_t StyleCount; reader->read_object_t(StyleCount, abortHandler);

        for (size_t i = 0; i < StyleCount; ++i)
        {
            uint32_t Id; reader->read_object_t(Id, abortHandler);

            if ((Version < 3) && (Id != 0)) // Version 2: VisualElement::GraphDescription was added before VisualElement::XAxisLine
                Id++;

            pfc::string Name = reader->read_string(abortHandler);

            uint64_t Flags; reader->read_object_t(Flags, abortHandler);
            uint32_t colorSource; reader->read_object_t(colorSource, abortHandler);
            D2D1_COLOR_F CustomColor; reader->read_object(&CustomColor, sizeof(CustomColor), abortHandler);
            uint32_t ColorIndex; reader->read_object_t(ColorIndex, abortHandler);
            uint32_t colorScheme; reader->read_object_t(colorScheme, abortHandler);

            GradientStops gs;

            size_t GradientStopCount; reader->read_object_t(GradientStopCount, abortHandler);

            for (size_t j = 0; j < GradientStopCount; ++j)
            {
                FLOAT Position; reader->read_object_t(Position, abortHandler);
                D2D1_COLOR_F Color; reader->read_object(&Color, sizeof(Color), abortHandler);

                gs.push_back({ Position, Color });
            }

            FLOAT Opacity; reader->read_object_t(Opacity, abortHandler);
            FLOAT Thickness; reader->read_object_t(Thickness, abortHandler);

            pfc::string FontName = reader->read_string(abortHandler);
            FLOAT FontSize; reader->read_object_t(FontSize, abortHandler);

            Style style = { Name, Flags, (ColorSource) colorSource, CustomColor, ColorIndex, (ColorScheme) colorScheme, gs, Opacity, Thickness, FontName, FontSize };

            _Styles.insert({ (VisualElement) Id, style });

            if ((Version < 3) && (Id == 0)) // Version 2: VisualElement::GraphDescription was added before VisualElement::XAxisLine
            {
                style = 
                {
                    "Graph Description", Style::SupportsOpacity | Style::SupportsFont,
                    ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
                };

                _Styles.insert({ VisualElement::GraphDescription, style });
            }
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while reading CUI styles: %s", core_api::get_my_file_name(), ex.what());

        Reset();
    }
}

/// <summary>
/// Writes this instance to a stream.
/// </summary>
void StyleManager::Write(stream_writer * writer, abort_callback & abortHandler) const noexcept
{
    try
    {
        writer->write_object_t(_CurrentVersion, abortHandler);

        size_t Size = _Styles.size();

        writer->write_object_t(Size, abortHandler);

        for (const auto & Iter : _Styles)
        {
            {
                uint32_t Id = (uint32_t) Iter.first;

                writer->write_object_t(Id, abortHandler);
            }

            {
                const Style & style = Iter.second;

                writer->write_string(style._Name, abortHandler);

                writer->write_object_t(style._Flags, abortHandler);
                writer->write_object(&style._ColorSource, sizeof(style._ColorSource), abortHandler);
                writer->write_object(&style._Color, sizeof(style._Color), abortHandler);
                writer->write_object_t(style._ColorIndex, abortHandler);
                writer->write_object(&style._ColorScheme, sizeof(style._ColorScheme), abortHandler);

                Size = style._CustomGradientStops.size();

                writer->write_object_t(Size, abortHandler);

                for (const auto & gs : style._CustomGradientStops)
                {
                    writer->write_object_t(gs.position, abortHandler);
                    writer->write_object(&gs.color, sizeof(gs.color), abortHandler);
                }

                writer->write_object_t(style._Opacity, abortHandler);
                writer->write_object_t(style._Thickness, abortHandler);

                writer->write_string(style._FontName, abortHandler);
                writer->write_object_t(style._FontSize, abortHandler);
            }
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing CUI styles: %s", core_api::get_my_file_name(), ex.what());
    }
}

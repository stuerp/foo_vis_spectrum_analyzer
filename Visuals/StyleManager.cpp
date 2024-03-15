
/** $VER: StyleManager.cpp (2024.03.15) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "StyleManager.h"

#include "Support.h"
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
        Iter.second._CurrentColor = Iter.second._CustomColor;
        Iter.second._CurrentGradientStops = GetGradientStops(Iter.second._ColorScheme);
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
/// Gets the style of the visual element specified by an index.
/// </summary>
Style * StyleManager::GetStyleByIndex(int index)
{
    static const VisualElement IndexToId[] =
    {
        VisualElement::GraphBackground,
        VisualElement::GraphDescriptionText,
        VisualElement::GraphDescriptionBackground,

        VisualElement::XAxisText,
        VisualElement::YAxisText,
        VisualElement::HorizontalGridLine,
        VisualElement::VerticalGridLine,

        VisualElement::BarArea,
        VisualElement::BarTop,
        VisualElement::BarPeakArea,
        VisualElement::BarPeakTop,
        VisualElement::BarDarkBackground,
        VisualElement::BarLightBackground,

        VisualElement::CurveLine,
        VisualElement::CurveArea,
        VisualElement::CurvePeakLine,
        VisualElement::CurvePeakArea,

        VisualElement::HeatMapForeground,
        VisualElement::HeatMapBackground,

        VisualElement::NyquistMarker,
    };

    index = Clamp(index, 0, (int) (_countof(IndexToId) - 1));

    return GetStyle(IndexToId[(size_t) index]);
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
                Iter.second._CurrentGradientStops = gs;
                Iter.second.ReleaseDeviceSpecificResources();
            }
        }
        else
        if (Iter.second._ColorSource == ColorSource::DominantColor)
        {
            Iter.second._CurrentColor = dominantColor;
            Iter.second.ReleaseDeviceSpecificResources();
        }
    }
}

/// <summary>
/// Updates the current color of each style.
/// </summary>
void StyleManager::UpdateCurrentColors()
{
    for (auto & Iter : _Styles)
        Iter.second.UpdateCurrentColor(_DominantColor, _UserInterfaceColors);
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

        size_t StyleCount; reader->read_object_t(StyleCount, abortHandler);

        for (size_t i = 0; i < StyleCount; ++i)
        {
            uint32_t Id; reader->read_object_t(Id, abortHandler);

            if ((Version < 3) && (Id != 0)) // Version 2: VisualElement::GraphDescription was added before VisualElement::XAxisLine
                Id++;

            if (Version < 4)
                pfc::string Name = reader->read_string(abortHandler);

            Style & style = _Styles[(VisualElement) Id];

            uint64_t Flags;

            reader->read_object_t(Flags, abortHandler);

            style._Flags = (style._Flags & Style::System) | (Flags & ~Style::System); // Retain the value of system style flags.

            uint32_t Integer;

            reader->read_object_t(Integer, abortHandler); style._ColorSource = (ColorSource) Integer;
            reader->read_object(&style._CustomColor, sizeof(style._CustomColor), abortHandler);
            reader->read_object_t(style._ColorIndex, abortHandler);
            reader->read_object_t(Integer, abortHandler); style._ColorScheme = (ColorScheme) Integer;

            GradientStops gs;

            size_t GradientStopCount; reader->read_object_t(GradientStopCount, abortHandler);

            for (size_t j = 0; j < GradientStopCount; ++j)
            {
                FLOAT Position; reader->read_object_t(Position, abortHandler);
                D2D1_COLOR_F Color; reader->read_object(&Color, sizeof(Color), abortHandler);

                gs.push_back({ Position, Color });
            }

            style._CustomGradientStops = gs;

            reader->read_object_t(style._Opacity, abortHandler);
            reader->read_object_t(style._Thickness, abortHandler);

            pfc::string FontName = reader->read_string(abortHandler);
            FLOAT FontSize; reader->read_object_t(FontSize, abortHandler);

            if (Version > 4)
            {
                style._FontName = pfc::wideFromUTF8(FontName);
                style._FontSize = FontSize;
            }

            // 'Activate' the values we just read.
            if (style._ColorScheme == ColorScheme::Custom)
                style._CurrentGradientStops = style._CustomGradientStops;
            else
                style._CurrentGradientStops = GetGradientStops(style._ColorScheme);

            style.UpdateCurrentColor(_DominantColor, _UserInterfaceColors);
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Failed to read styles: %s", core_api::get_my_file_name(), ex.what());

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

                writer->write_object_t(style._Flags, abortHandler);
                writer->write_object(&style._ColorSource, sizeof(style._ColorSource), abortHandler);
                writer->write_object(&style._CurrentColor, sizeof(style._CurrentColor), abortHandler);
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

                pfc::string FontName = pfc::utf8FromWide(style._FontName.c_str());
                writer->write_string(FontName, abortHandler);
                writer->write_object_t(style._FontSize, abortHandler);
            }
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Failed to write styles: %s", core_api::get_my_file_name(), ex.what());
    }
}

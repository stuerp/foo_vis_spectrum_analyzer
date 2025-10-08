
/** $VER: StyleManager.cpp (2025.09.22) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "pch.h"
#include "StyleManager.h"

#include "Support.h"
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
    for (const auto & Iter : other._Styles)
        _Styles[Iter.first] = Iter.second;

    return *this;
}

/// <summary>
/// Resets this instance.
/// </summary>
void style_manager_t::Reset() noexcept
{
    _Styles = _DefaultStyles;

    for (auto & Iter : _Styles)
    {
        Iter.second._CurrentColor = Iter.second._CustomColor;
        Iter.second._CurrentGradientStops = GetBuiltInGradientStops(Iter.second._ColorScheme);
    }
}

/// <summary>
/// Gets the style of the visual element specified by an index.
/// </summary>
style_t * style_manager_t::GetStyleByIndex(int index) noexcept
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

        VisualElement::Spectogram,

        VisualElement::GaugeBackground,

        VisualElement::GaugePeakLevel,
        VisualElement::Gauge0dBPeakLevel,
        VisualElement::GaugeMaxPeakLevel,
        VisualElement::GaugePeakLevelText,

        VisualElement::GaugeRMSLevel,
        VisualElement::Gauge0dBRMSLevel,
        VisualElement::GaugeRMSLevelText,

        VisualElement::NyquistMarker,

        VisualElement::GaugeLeftRight,
        VisualElement::GaugeLeftRightIndicator,
        VisualElement::GaugeMidSide,
        VisualElement::GaugeMidSideIndicator,
        VisualElement::LevelMeterAxis,

        VisualElement::SignalLine,
    };

    static_assert((size_t) VisualElement::Count == _countof(IndexToId), "Index too small");

    index = std::clamp(index, 0, (int) (_countof(IndexToId) - 1));

    return GetStyle(IndexToId[(size_t) index]);
}

/// <summary>
/// Updates the style parameters of all styles that are using the artwork as source.
/// </summary>
void style_manager_t::SetArtworkDependentParameters(const gradient_stops_t & gs, D2D1_COLOR_F dominantColor) noexcept
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
void style_manager_t::UpdateCurrentColors() noexcept
{
    for (auto & Iter : _Styles)
        Iter.second.UpdateCurrentColor(_DominantColor, _UserInterfaceColors);
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void style_manager_t::ReleaseDeviceSpecificResources() noexcept
{
    for (auto & Iter : _Styles)
        Iter.second.ReleaseDeviceSpecificResources();
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

            if (Id < (uint32_t) VisualElement::Count)
                Style = _Styles[(VisualElement) Id];

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
;
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

            Style.UpdateCurrentColor(_DominantColor, _UserInterfaceColors);

            if (Id < (uint32_t) VisualElement::Count)
                _Styles[(VisualElement) Id] = Style;
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

        size_t Size = _Styles.size();

        writer->write_object_t(Size, abortHandler);

        for (const auto & Iter : _Styles)
        {
            {
                uint32_t Id = (uint32_t) Iter.first;

                writer->write_object_t(Id, abortHandler);
            }

            {
                const style_t & style = Iter.second;

                writer->write_object_t((uint64_t) style._Flags, abortHandler);
                writer->write_object(&style._ColorSource, sizeof(style._ColorSource), abortHandler);
                writer->write_object(&style._CustomColor, sizeof(style._CustomColor), abortHandler);
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
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to write styles: %s", ex.what());
    }
}

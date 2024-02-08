
/** $VER: StyleManager.cpp (2024.02.07) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "StyleManager.h"

#include "Gradients.h"
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
    _Styles.clear();

    _Styles.insert
    ({
        VisualElement::Background,
        {
            "Background", Style::SupportsOpacity,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::XAxisLine,
        {
            "X-axis Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::XAxisText,
        {
            "X-axis Text", Style::SupportsOpacity | Style::SupportsFont,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::YAxisLine,
        {
            "Y-axis Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::YAxisText,
        {
            "Y-axis Text", Style::SupportsOpacity | Style::SupportsFont,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarSpectrum,
        {
            "Bar Spectrum", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarDarkBackground,
        {
            "Bar Dark Background", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarLightBackground,
        {
            "Bar Light Background", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarPeakIndicator,
        {
            "Bar Peak Indicator", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurveLine,
        {
            "Curve Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurveArea,
        {
            "Curve Area", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .5f, 0.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurvePeakLine,
        {
            "Curve Peak Line", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f, "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurvePeakArea,
        {
            "Curve Peak Area", Style::SupportsOpacity | Style::SupportsThickness,
            ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .25f, 0.f, "", 0.f,
        }
    });

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

void StyleManager::Read(ui_element_config_parser & parser) noexcept
{
    Reset();

    try
    {
        uint32_t Version;

        parser >> Version;

        if (Version > _CurrentVersion)
            return;

        size_t StyleCount;

        parser >> StyleCount;

        _Styles.clear();

        for (size_t i = 0; i < StyleCount; ++i)
        {
            uint32_t Id; parser >> Id;

            pfc::string Name; parser >> Name;

            size_t Flags; parser >> Flags;
            uint32_t colorSource; parser >> colorSource;

            D2D1_COLOR_F CustomColor = { };

            parser >> CustomColor.r;
            parser >> CustomColor.g;
            parser >> CustomColor.b;
            parser >> CustomColor.a;

            uint32_t ColorIndex; parser >> ColorIndex;
            uint32_t colorScheme; parser >> colorScheme;

            GradientStops gs;

            size_t GradientStopCount; parser >> GradientStopCount;

            for (size_t j = 0; j < GradientStopCount; ++j)
            {
                FLOAT Position; parser >> Position;

                D2D1_COLOR_F Color = { };

                parser >> Color.r;
                parser >> Color.g;
                parser >> Color.b;
                parser >> Color.a;

                gs.push_back({ Position, Color });;
            }

            FLOAT Opacity; parser >> Opacity;
            FLOAT Thickness; parser >> Thickness;

            pfc::string FontName; parser >> FontName;
            FLOAT FontSize; parser >> FontSize;

            Style style = { Name, Flags, (ColorSource) colorSource, CustomColor, ColorIndex, (ColorScheme) colorScheme, gs, Opacity, Thickness, FontName, FontSize };

            _Styles.insert({ (VisualElement) Id, style });
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while reading DUI styles: %s", core_api::get_my_file_name(), ex.what());

        Reset();
    }
}

void StyleManager::Write(ui_element_config_builder & builder) const noexcept
{
    try
    {
        builder << _CurrentVersion;

        builder << _Styles.size();

        for (const auto & Iter : _Styles)
        {
            builder << (uint32_t) Iter.first;

            const Style & style = Iter.second;

            builder << style._Name;
            builder << style._Flags;

            builder << (uint32_t) style._ColorSource;

            builder << style._CustomColor.r;
            builder << style._CustomColor.g;
            builder << style._CustomColor.b;
            builder << style._CustomColor.a;

            builder << style._ColorIndex;
            builder << (uint32_t) style._ColorScheme;

            builder << style._CustomGradientStops.size();

            for (const auto & gs : style._CustomGradientStops)
            {
                builder << gs.position;
                builder << gs.color.r;
                builder << gs.color.g;
                builder << gs.color.b;
                builder << gs.color.a;
            }

            builder << style._Opacity;
            builder << style._Thickness;

            builder << style._FontName;
            builder << style._FontSize;
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing DUI styles: %s", core_api::get_my_file_name(), ex.what());
    }
}

void StyleManager::Read(stream_reader * reader, size_t size, abort_callback & abortHandler) noexcept
{
    try
    {
        _Styles.clear();

        uint32_t Version;

        reader->read(&Version, sizeof(Version), abortHandler);

        if (Version > _CurrentVersion)
            return;

        size_t StyleCount;

        reader->read(&StyleCount, sizeof(StyleCount), abortHandler);

        for (size_t i = 0; i < StyleCount; ++i)
        {
            uint32_t Id; reader->read(&Id, sizeof(Id), abortHandler);

            pfc::string Name; Name = reader->read_string(abortHandler);

            size_t Flags; reader->read(&Flags, sizeof(Flags), abortHandler);
            uint32_t colorSource; reader->read(&colorSource, sizeof(colorSource), abortHandler);
            D2D1_COLOR_F CustomColor; reader->read(&CustomColor, sizeof(CustomColor), abortHandler);
            uint32_t ColorIndex; reader->read(&ColorIndex, sizeof(ColorIndex), abortHandler);
            uint32_t colorScheme; reader->read(&colorScheme, sizeof(colorScheme), abortHandler);

            GradientStops gs;

            size_t GradientStopCount; reader->read(&GradientStopCount, sizeof(GradientStopCount), abortHandler);

            for (size_t j = 0; j < GradientStopCount; ++j)
            {
                FLOAT Position; reader->read(&Position, sizeof(Position), abortHandler);
                D2D1_COLOR_F Color; reader->read(&Color, sizeof(Color), abortHandler);

                gs.push_back({ Position, Color });
            }

            FLOAT Opacity; reader->read(&Opacity, sizeof(Opacity), abortHandler);
            FLOAT Thickness; reader->read(&Thickness, sizeof(Thickness), abortHandler);

            pfc::string FontName = reader->read_string(abortHandler);
            FLOAT FontSize; reader->read(&FontSize, sizeof(FontSize), abortHandler);

            Style style = { Name, Flags, (ColorSource) colorSource, CustomColor, ColorIndex, (ColorScheme) colorScheme, gs, Opacity, Thickness, FontName, FontSize };

            _Styles.insert({ (VisualElement) Id, style });
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while reading CUI styles: %s", core_api::get_my_file_name(), ex.what());

        Reset();
    }
}

void StyleManager::Write(stream_writer * writer, abort_callback & abortHandler) const noexcept
{
    try
    {
        writer->write(&_CurrentVersion, sizeof(_CurrentVersion), abortHandler);

        size_t Size = _Styles.size();

        writer->write(&Size, sizeof(Size), abortHandler);

        for (const auto & Iter : _Styles)
        {
            uint32_t Id = (uint32_t) Iter.first;

            writer->write(&Id, sizeof(Id), abortHandler);

            const Style & style = Iter.second;

            writer->write_string(style._Name, abortHandler);

            writer->write(&style._Flags, sizeof(style._Flags), abortHandler);

            writer->write(&style._ColorSource, sizeof(style._ColorSource), abortHandler);
            writer->write(&style._Color, sizeof(style._Color), abortHandler);
            writer->write(&style._ColorIndex, sizeof(style._ColorIndex), abortHandler);
            writer->write(&style._ColorScheme, sizeof(style._ColorScheme), abortHandler);

            Size = style._CustomGradientStops.size();

            writer->write(&Size, sizeof(Size), abortHandler);

            for (const auto & gs : style._CustomGradientStops)
            {
                writer->write(&gs.position, sizeof(gs.position), abortHandler);
                writer->write(&gs.color, sizeof(gs.color), abortHandler);
            }

            writer->write(&style._Opacity, sizeof(style._Opacity), abortHandler);
            writer->write(&style._Thickness, sizeof(style._Thickness), abortHandler);

            writer->write_string(style._FontName, abortHandler);
            writer->write(&style._FontSize, sizeof(style._FontSize), abortHandler);
        }
    }
    catch (std::exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing CUI styles: %s", core_api::get_my_file_name(), ex.what());
    }
}


/** $VER: StyleManager.cpp (2024.02.04) P. Stuer - Creates and manages the DirectX resources of the styles. **/

#include "StyleManager.h"

#include "Gradients.h"
#include "Log.h"

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
            "Background", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::Black), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::XAxisLine,
        {
            "X-axis Line", ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::XAxisText,
        {
            "X-axis Text", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::YAxisLine,
        {
            "Y-axis Line", ColorSource::Solid, D2D1::ColorF(.25f, .25f, .25f, 1.f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::YAxisText,
        {
            "Y-axis Text", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarForeground,
        {
            "Bar Foreground", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Prism1, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarDarkBackground,
        {
            "Bar Dark Background", ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarLightBackground,
        {
            "Bar Light Background", ColorSource::Solid, D2D1::ColorF(.2f, .2f, .2f, .7f), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 0.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::BarPeakIndicator,
        {
            "Bar Peak Indicator", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Solid, GetGradientStops(ColorScheme::Custom), 1.f, 1.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurveLine,
        {
            "Curve Line", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurveArea,
        {
            "Curve Area", ColorSource::Gradient, D2D1::ColorF(0), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .5f, 0.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurvePeakLine,
        {
            "Curve Peak Line", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), 1.f, 2.f,
            "", 0.f,
        }
    });

    _Styles.insert
    ({
        VisualElement::CurvePeakArea,
        {
            "Curve Peak Area", ColorSource::Solid, D2D1::ColorF(D2D1::ColorF::White), 0, ColorScheme::Artwork, GetGradientStops(ColorScheme::Custom), .25f, 0.f,
            "", 0.f,
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
                Iter.second._Brush.Release();
            }
        }
        else
        if (Iter.second._ColorSource == ColorSource::DominantColor)
        {
            Iter.second._Color = dominantColor;
            Iter.second._Brush.Release();
        }
    }
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void StyleManager::ReleaseDeviceSpecificResources()
{
    for (auto & Iter : _Styles)
        Iter.second._Brush.Release();
}

void StyleManager::Read(ui_element_config_parser & parser) noexcept
{
    Reset();

    try
    {
        size_t Version;

        parser >> Version;

        if (Version > _CurrentVersion)
            return;

        size_t StyleCount;

        parser >> StyleCount;

        _Styles.clear();

        for (size_t i = 0; i < StyleCount; ++i)
        {
            int Id; parser >> Id;

            pfc::string Name; parser >> Name;

            int colorSource; parser >> colorSource;

            D2D1_COLOR_F CustomColor;

            parser >> CustomColor.r;
            parser >> CustomColor.g;
            parser >> CustomColor.b;
            parser >> CustomColor.a;

            int ColorIndex; parser >> ColorIndex;
            int colorScheme; parser >> colorScheme;

            GradientStops gs;

            size_t GradientStopCount; parser >> GradientStopCount;

            for (size_t j = 0; j < GradientStopCount; ++j)
            {
                FLOAT Position; parser >> Position;

                D2D1_COLOR_F Color;

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

            Style style = { Name, (ColorSource) colorSource, CustomColor, ColorIndex, (ColorScheme) colorScheme, gs, Opacity, Thickness, FontName, FontSize };

            style._Color = style._CustomColor;
            style._GradientStops = style._CustomGradientStops;

            _Styles.insert({ (VisualElement) Id, style });
        }
    }
    catch (exception_io & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while reading DUI configuration data: %s", core_api::get_my_file_name(), ex.what());

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
            builder << (int) Iter.first;

            const Style & style = Iter.second;

            builder << style._Name;

            builder << (int) style._ColorSource;

            builder << style._CustomColor.r;
            builder << style._CustomColor.g;
            builder << style._CustomColor.b;
            builder << style._CustomColor.a;

            builder << style._ColorIndex;
            builder << (int) style._ColorScheme;

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
    catch (exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing DUI styles: %s", core_api::get_my_file_name(), ex.what());
    }
}

void StyleManager::Read(stream_reader * reader, size_t size, abort_callback & abortHandler) noexcept
{
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
            int Index = (int) Iter.first;

            writer->write(&Index, sizeof(Index), abortHandler);

            const Style & style = Iter.second;

            Size = style._Name.length();
            writer->write(&Size, sizeof(Size), abortHandler);
            writer->write(style._Name.c_str(), Size + 1, abortHandler);

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

            Size = style._FontName.length();
            writer->write(&Size, sizeof(Size), abortHandler);
            writer->write(style._FontName.c_str(), Size + 1, abortHandler);

            writer->write(&style._FontSize, sizeof(style._FontSize), abortHandler);
        }
    }
    catch (exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing CUI styles: %s", core_api::get_my_file_name(), ex.what());
    }
}

StyleManager _StyleManager;

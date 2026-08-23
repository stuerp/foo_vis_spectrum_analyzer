
/** $VER: GraphOptions.cpp (2026.08.22) P. Stuer - Describes the layout and settings of a graph. **/

#include "pch.h"

#include "GraphOptions.h"

#include "Constants.h"
#include "Log.h"
#include "Resources.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Resets this instance.
/// </summary>
void graph_options_t::Reset() noexcept
{
    _SelectedChannels        = (uint32_t) Channels::ConfigStereo;

    _ChannelPair             = ChannelPair::FrontLeftRight;
    _SwapChannels            = false;

    _HorizontalAlignment     = HorizontalAlignment::Center;
    _VerticalAlignment       = VerticalAlignment::Center;

    _FlipHorizontally        = false;
    _FlipVertically          = false;

    _XAxisMode               = XAxisMode::Bands;
    _XAxisTop                = false;
    _XAxisBottom             = true;
    _XAxisDecimals           = 3;

    _YAxisMode               = YAxisMode::Decibels;
    _YAxisLeft               = true;
    _YAxisRight              = false;

    _AmplitudeLo             = -90.;    // Lower amplitude, [-120, 0]
    _AmplitudeHi             =   0.;    // Upper amplitude, [-120, 0]
    _AmplitudeStep           =  -6.;

    _UseAbsolute             = true;    // Linear/n-th root scaling: Sets the min. dB range to -∞ dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
    _Gamma                   = 1.;      // Linear/n-th root scaling: Index n of the n-th root calculation, [0.5, 10]

    _HRatio                  = 1.f;
    _VRatio                  = 1.f;

    _LPadding                = 0.f;
    _RPadding                = 0.f;
    _TPadding                = 0.f;
    _BPadding                = 0.f;

    _HorizontalTextAlignment = HorizontalTextAlignment::Center;
    _VerticalTextAlignment   = VerticalTextAlignment::Center;

    _UseLocalStyles          = false;
}

/// <summary>
/// Scales the specified value to a relative amplitude between 0 and 1.
/// </summary>
double graph_options_t::ScaleAmplitude(double value) const noexcept
{
    switch (_YAxisMode)
    {
        default:

        case YAxisMode::None:

        case YAxisMode::Decibels:
            return msc::Map(ToDecibel(value), _AmplitudeLo, _AmplitudeHi, 0., 1.);

        case YAxisMode::Linear:
        {
            const double Exponent = 1. / _Gamma;

            return msc::Map(::pow(value, Exponent), _UseAbsolute ? 0. : ::pow(ToMagnitude(_AmplitudeLo), Exponent), ::pow(ToMagnitude(_AmplitudeHi), Exponent), 0., 1.);
        }
    }
}

/// <summary>
/// Deserializes this instance.
/// </summary>
graph_options_t graph_options_t::FromJSON(const json & object) noexcept
{
    graph_options_t Options;

    try
    {
        const auto & Description = object.value("description", json::object());
        {
            Options._Description             = msc::UTF8ToWide(Description.value("text", msc::WideToUTF8(Options._Description)));

            Options._HorizontalTextAlignment = std::clamp((HorizontalTextAlignment) Description.value("horizontalAlignment", Options._HorizontalTextAlignment), HorizontalTextAlignment::Min, HorizontalTextAlignment::Max);
            Options._VerticalTextAlignment   = std::clamp((VerticalTextAlignment)   Description.value("verticalAlignment",   Options._VerticalTextAlignment),   VerticalTextAlignment::Min,   VerticalTextAlignment::Max);
        }

        Options._SelectedChannels        = std::clamp((uint32_t) object.value("channels", Options._SelectedChannels), (uint32_t) Channels::None, (uint32_t) Channels::All);
        Options._ChannelPair             = std::clamp((ChannelPair) object.value("channelPair", Options._ChannelPair), ChannelPair::Min, ChannelPair::Max);
        Options._SwapChannels            = object.value("swapChannels", Options._SwapChannels);

        const auto & Layout = object.value("layout", json::object());
        {
            Options._HorizontalAlignment     = std::clamp((HorizontalAlignment) Layout.value("horizontalAlignment", Options._HorizontalAlignment), HorizontalAlignment::Min, HorizontalAlignment::Max);
            Options._HRatio                  = std::clamp((FLOAT) Layout.value("horizontalRatio", Options._HRatio), 0.f, 1.f);
            Options._VerticalAlignment       = std::clamp((VerticalAlignment) Layout.value("verticalAlignment", Options._VerticalAlignment), VerticalAlignment::Min, VerticalAlignment::Max);
            Options._VRatio                  = std::clamp((FLOAT) Layout.value("verticalRatio", Options._VRatio), 0.f, 1.f);

            Options._FlipHorizontally        = Layout.value("flipHorizontally", Options._FlipHorizontally);
            Options._FlipVertically          = Layout.value("flipVertically", Options._FlipVertically);

            Options._LPadding                = std::clamp(Layout.value("leftPadding",   Options._LPadding), 0.f, 0.f);  // Not configurable yet.
            Options._RPadding                = std::clamp(Layout.value("rightPadding",  Options._RPadding), 0.f, 0.f);  // Not configurable yet.
            Options._TPadding                = std::clamp(Layout.value("topPadding",    Options._TPadding), 0.f, 0.f);  // Not configurable yet.
            Options._BPadding                = std::clamp(Layout.value("bottomPadding", Options._BPadding), 0.f, 0.f);  // Not configurable yet.
        }

        const auto & XAxis = object.value("xAxis", json::object());
        {
            Options._XAxisMode               = std::clamp((XAxisMode) XAxis.value("mode", Options._XAxisMode), XAxisMode::Min, XAxisMode::Max);
            Options._XAxisTop                = XAxis.value("top",    Options._XAxisTop);
            Options._XAxisBottom             = XAxis.value("bottom", Options._XAxisBottom);
            Options._XAxisDecimals           = (uint32_t) std::clamp((int) XAxis.value("decimals", Options._XAxisDecimals), MinXAxisDecimals, MaxXAxisDecimals);
        }

        const auto & YAxis = object.value("yAxis", json::object());
        {
            Options._YAxisMode               = std::clamp((YAxisMode) YAxis.value("mode", Options._YAxisMode), YAxisMode::Min, YAxisMode::Max);
            Options._YAxisLeft               = YAxis.value("left",  Options._YAxisLeft);
            Options._YAxisRight              = YAxis.value("right", Options._YAxisRight);

            Options._AmplitudeLo             = std::clamp(YAxis.value("amplitudeLo",   Options._AmplitudeLo),   MinAmplitude,     MaxAmplitude);
            Options._AmplitudeHi             = std::clamp(YAxis.value("amplitudeHi",   Options._AmplitudeHi),   MinAmplitude,     MaxAmplitude);
            Options._AmplitudeStep           = std::clamp(YAxis.value("amplitudeStep", Options._AmplitudeStep), MinAmplitudeStep, MaxAmplitudeStep);

            Options._UseAbsolute             = YAxis.value("useAbsolute", Options._UseAbsolute);
            Options._Gamma                   = std::clamp(YAxis.value("gamma", Options._Gamma), MinGamma, MaxGamma);
        }

        const auto & Styles = object.value("styles", json::array());
        {
            Options._StyleManager.FromJSON(Styles);
        }

        Options._UseLocalStyles = object.value("useLocalStyles", Options._UseLocalStyles);
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to deserialize graph options: %s", e.what());

        Options.Reset();
    }

    return Options;
}

/// <summary>
/// Serializes this instance to JSON string.
/// </summary>
json graph_options_t::ToJSON() const noexcept
{
    try
    {
        json Object =
        {
            { "description",
                json::object
                ({
                    { "text", msc::WideToUTF8(_Description) },
                    { "horizontalAlignment", _HorizontalTextAlignment },
                    { "verticalAlignment", _VerticalTextAlignment },
                })
            },

            { "channels", _SelectedChannels },
            { "channelPair", _ChannelPair },
            { "swapChannels", _SwapChannels },

            { "layout",
                json::object
                ({
                    { "horizontalAlignment", _HorizontalAlignment },
                    { "horizontalRatio", _HRatio },
                    { "verticalAlignment", _VerticalAlignment },
                    { "verticalRatio", _VRatio },

                    { "flipHorizontally", _FlipHorizontally },
                    { "flipVertically", _FlipVertically },

                    { "leftPadding", _LPadding },
                    { "rightPadding", _RPadding },
                    { "topPadding", _TPadding },
                    { "bottomPadding", _BPadding },
                })
            },

            { "xAxis",
                json::object
                ({
                    { "mode", _XAxisMode },
                    { "top", _XAxisTop },
                    { "bottom", _XAxisBottom },
                    { "decimals", _XAxisDecimals },
                })
            },

            { "yAxis",
                json::object
                ({
                    { "mode", _YAxisMode },
                    { "left", _YAxisLeft },
                    { "right", _YAxisRight },

                    { "amplitudeLo", _AmplitudeLo },
                    { "amplitudeHi", _AmplitudeHi },
                    { "amplitudeStep", _AmplitudeStep },

                    { "useAbsolute", _UseAbsolute },
                    { "gamma", _Gamma },
                })
            },

            { "styles", _StyleManager.ToJSON() },
            { "useLocalStyles", _UseLocalStyles },
        };

        return Object;
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to serialize graph options: %s", e.what());

        return { };
    }
}

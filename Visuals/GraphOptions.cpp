
/** $VER: GraphOptions.cpp (2026.06.17) P. Stuer - Describes the layout and settings of a graph. **/

#include "pch.h"

#include "Constants.h"
#include "GraphOptions.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void graph_options_t::Initialize() noexcept
{
    _SelectedChannels = (uint32_t) Channels::ConfigStereo;

    _ChannelPair  = ChannelPair::FrontLeftRight;
    _SwapChannels = false;

    _HorizontalAlignment = HorizontalAlignment::Center;
    _VerticalAlignment = VerticalAlignment::Center;

    _FlipHorizontally = false;
    _FlipVertically = false;

    _XAxisMode = XAxisMode::Bands;
    _XAxisTop = false;
    _XAxisBottom = true;
    _XAxisDecimals = 3;

    _YAxisMode = YAxisMode::Decibels;
    _YAxisLeft = true;
    _YAxisRight = false;

    _AmplitudeLo =  -90.;    // Lower amplitude, [-120, 0]
    _AmplitudeHi =    0.;    // Upper amplitude, [-120, 0]
    _AmplitudeStep = -6.;

    _UseAbsolute = true;    // Linear/n-th root scaling: Sets the min. dB range to -∞ dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
    _Gamma = 1.;            // Linear/n-th root scaling: Index n of the n-th root calculation, [0.5, 10.0]

    _HRatio = 1.f;
    _VRatio = 1.f;

    _LPadding = 0.f;
    _RPadding = 0.f;
    _TPadding = 0.f;
    _BPadding = 0.f;

    _HorizontalTextAlignment = HorizontalTextAlignment::Center;
    _VerticalTextAlignment = VerticalTextAlignment::Center;
}

/// <summary>
/// Scales the specified value to a relative amplitude between 0.0 and 1.0.
/// </summary>
double graph_options_t::ScaleAmplitude(double value) const noexcept
{
    switch (_YAxisMode)
    {
        default:

        case YAxisMode::None:

        case YAxisMode::Decibels:
            return msc::Map(ToDecibel(value), _AmplitudeLo, _AmplitudeHi, 0.0, 1.0);

        case YAxisMode::Linear:
        {
            const double Exponent = 1.0 / _Gamma;

            return msc::Map(::pow(value, Exponent), _UseAbsolute ? 0.0 : ::pow(ToMagnitude(_AmplitudeLo), Exponent), ::pow(ToMagnitude(_AmplitudeHi), Exponent), 0.0, 1.0);
        }
    }
}

/// <summary>
/// Deserializes this instance.
/// </summary>
graph_options_t graph_options_t::FromJSON(const json & object) noexcept
{
    graph_options_t Options;

    const auto & Description = object.value("description", json::object());

    Options._Description             = msc::UTF8ToWide(Description.value("text", msc::WideToUTF8(Options._Description)));

    Options._HorizontalTextAlignment = Description.value("horizontalAlignment", Options._HorizontalTextAlignment);
    Options._VerticalTextAlignment   = Description.value("verticalAlignment", Options._VerticalTextAlignment);

    Options._SelectedChannels = object.value("channels", Options._SelectedChannels);
    Options._ChannelPair      = object.value("channelPair", Options._ChannelPair);
    Options._SwapChannels     = object.value("swapChannels", Options._SwapChannels);

    const auto & Layout = object.value("layout", json::object());

    Options._HorizontalAlignment = Layout.value("horizontalAlignment", Options._HorizontalAlignment);
    Options._HRatio              = Layout.value("horizontalRatio", Options._HRatio);
    Options._VerticalAlignment   = Layout.value("verticalAlignment", Options._VerticalAlignment);
    Options._VRatio              = Layout.value("verticalRatio", Options._VRatio);

    Options._FlipHorizontally = Layout.value("flipHorizontally", Options._FlipHorizontally);
    Options._FlipVertically   = Layout.value("flipVertically", Options._FlipVertically);

    Options._LPadding = Layout.value("leftPadding", Options._LPadding);
    Options._RPadding = Layout.value("rightPadding", Options._RPadding);
    Options._TPadding = Layout.value("topPadding", Options._TPadding);
    Options._BPadding = Layout.value("bottomPadding", Options._BPadding);

    const auto & XAxis = object.value("xAxis", json::object());

    Options._XAxisMode     = XAxis.value("mode", Options._XAxisMode);
    Options._XAxisTop      = XAxis.value("top", Options._XAxisTop);
    Options._XAxisBottom   = XAxis.value("bottom", Options._XAxisBottom);
    Options._XAxisDecimals = XAxis.value("decimals", Options._XAxisDecimals);

    const auto & YAxis = object.value("yAxis", json::object());

    Options._YAxisMode     = YAxis.value("mode", Options._YAxisMode);
    Options._YAxisLeft     = YAxis.value("left", Options._YAxisLeft);
    Options._YAxisRight    = YAxis.value("right", Options._YAxisRight);

    Options._AmplitudeLo   = YAxis.value("amplitudeLo", Options._AmplitudeLo);
    Options._AmplitudeHi   = YAxis.value("amplitudeHi", Options._AmplitudeHi);
    Options._AmplitudeStep = YAxis.value("amplitudeStep", Options._AmplitudeStep);

    Options._UseAbsolute   = YAxis.value("useAbsolute", Options._UseAbsolute);
    Options._Gamma         = YAxis.value("gamma", Options._Gamma);

    const auto & Styles = object.value("styles", json::array());

    Options._StyleManager.FromJSON(Styles);

    return Options;
}

/// <summary>
/// Serializes this instance to JSON string.
/// </summary>
json graph_options_t::ToJSON() const noexcept
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
    };

    return Object;
}

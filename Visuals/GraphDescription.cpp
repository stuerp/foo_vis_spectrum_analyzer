
/** $VER: GraphDescription.cpp (2026.03.21) P. Stuer - Describes the layout and settings of a graph. **/

#include "pch.h"

#include "GraphDescription.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Scales the specified value to a relative amplitude between 0.0 and 1.0.
/// </summary>
double graph_description_t::ScaleAmplitude(double value) const noexcept
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
/// Serializes this instance to JSON string.
/// </summary>
json graph_description_t::ToJSON() const noexcept
{
    json Object =
    {
        { "description",
            json::object
            ({
                { "text", msc::WideToUTF8(_Description) },
                { "horizontalAlignment", _HAlignment },
                { "verticalAlignment", _VAlignment },
            })
        },

        { "channels", _SelectedChannels },
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
    };

    return Object;
}

/// <summary>
/// Deserializes this instance.
/// </summary>
graph_description_t graph_description_t::FromJSON(const json & object) noexcept
{
    graph_description_t gd;

    const auto & Description = object.value("description", json::object());

    gd._Description = msc::UTF8ToWide(Description.value("text", msc::WideToUTF8(gd._Description)));
    gd._HAlignment  = Description.value("horizontalAlignment", gd._HAlignment);
    gd._VAlignment  = Description.value("verticalAlignment", gd._VAlignment);

    gd._SelectedChannels = object.value("channels", gd._SelectedChannels);
    gd._SwapChannels     = object.value("swapChannels", gd._SwapChannels);

    const auto & Layout = object.value("layout", json::object());

    gd._HorizontalAlignment = Layout.value("horizontalAlignment", gd._HorizontalAlignment);
    gd._HRatio              = Layout.value("horizontalRatio", gd._HRatio);
    gd._VerticalAlignment   = Layout.value("verticalAlignment", gd._VerticalAlignment);
    gd._VRatio              = Layout.value("verticalRatio", gd._VRatio);

    gd._FlipHorizontally = Layout.value("flipHorizontally", gd._FlipHorizontally);
    gd._FlipVertically   = Layout.value("flipVertically", gd._FlipVertically);

    gd._LPadding = Layout.value("leftPadding", gd._LPadding);
    gd._RPadding = Layout.value("rightPadding", gd._RPadding);
    gd._TPadding = Layout.value("topPadding", gd._TPadding);
    gd._BPadding = Layout.value("bottomPadding", gd._BPadding);

    const auto & XAxis = object.value("xAxis", json::object());

    gd._XAxisMode     = XAxis.value("mode", gd._XAxisMode);
    gd._XAxisTop      = XAxis.value("top", gd._XAxisTop);
    gd._XAxisBottom   = XAxis.value("bottom", gd._XAxisBottom);
    gd._XAxisDecimals = XAxis.value("decimals", gd._XAxisDecimals);

    const auto & YAxis = object.value("yAxis", json::object());

    gd._YAxisMode     = YAxis.value("mode", gd._YAxisMode);
    gd._YAxisLeft     = YAxis.value("top", gd._YAxisLeft);
    gd._YAxisRight    = YAxis.value("bottom", gd._YAxisRight);

    gd._AmplitudeLo   = YAxis.value("amplitudeLo", gd._AmplitudeLo);
    gd._AmplitudeHi   = YAxis.value("amplitudeHi", gd._AmplitudeHi);
    gd._AmplitudeStep = YAxis.value("amplitudeStep", gd._AmplitudeStep);

    gd._UseAbsolute   = YAxis.value("useAbsolute", gd._UseAbsolute);
    gd._Gamma         = YAxis.value("gamma", gd._Gamma);

    return gd;
}

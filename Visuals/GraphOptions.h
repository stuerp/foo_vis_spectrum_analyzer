
/** $VER: GraphOptions.h (2026.06.15) P. Stuer - Describes the layout and settings of a graph. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Constants.h"
#include "StyleManager.h"

#include <string>

#pragma warning(push)
#pragma warning(disable: 4868) // compiler may not enforce left-to-right evaluation order in braced initializer list

#include <nlohmann\json.hpp>

using json = nlohmann::ordered_json;

#pragma warning(pop)

/// <summary>
/// Represents the options of a graph.
/// </summary>
struct graph_options_t
{
public:
    graph_options_t()
    {
        Initialize();
    }

    graph_options_t(const std::wstring & description)
    {
        _Description = description;

        Initialize();
    }

    graph_options_t(const graph_options_t & other)
    {
        *this = other;
    }

    graph_options_t & operator=(const graph_options_t & other)
    {
        _Description         = other._Description;

        _SelectedChannels    = other._SelectedChannels;

        _ChannelPair         = other._ChannelPair;
        _SwapChannels        = other._SwapChannels;

        _HorizontalAlignment = other._HorizontalAlignment;
        _VerticalAlignment   = other._VerticalAlignment;

        _FlipHorizontally    = other._FlipHorizontally;
        _FlipVertically      = other._FlipVertically;

        _XAxisMode           = other._XAxisMode;
        _XAxisTop            = other._XAxisTop;
        _XAxisBottom         = other._XAxisBottom;
        _XAxisDecimals       = other._XAxisDecimals;

        _YAxisMode           = other._YAxisMode;
        _YAxisLeft           = other._YAxisLeft;
        _YAxisRight          = other._YAxisRight;

        _AmplitudeLo         = other._AmplitudeLo;
        _AmplitudeHi         = other._AmplitudeHi;
        _AmplitudeStep       = other._AmplitudeStep;

        _UseAbsolute         = other._UseAbsolute;
        _Gamma               = other._Gamma;

        _HRatio              = other._HRatio;
        _VRatio              = other._VRatio;

        _LPadding            = other._LPadding;
        _RPadding            = other._RPadding;
        _TPadding            = other._TPadding;
        _BPadding            = other._BPadding;

        _HorizontalTextAlignment = other._HorizontalTextAlignment;
        _VerticalTextAlignment   = other._VerticalTextAlignment;

        _StyleManager            = other._StyleManager;

        return *this;
    }

    virtual ~graph_options_t() { }

    double ScaleAmplitude(double value) const noexcept;

    /* Code readability shortcuts */
    bool HasXAxis() const noexcept { return _XAxisMode != XAxisMode::None; }
    bool HasYAxis() const noexcept { return _YAxisMode != YAxisMode::None; }

    json ToJSON() const noexcept;
    static graph_options_t FromJSON(const json & object) noexcept;

private:
    void Initialize() noexcept;

public:
    std::wstring _Description;

    uint32_t _SelectedChannels;                 // The channels that should be visualized.

    ChannelPair _ChannelPair;
    bool _SwapChannels;                         // True if the channels of a channel pair should be swapped.

    HorizontalAlignment _HorizontalAlignment;   // Horizonal alignment of a visualization in the graph area.
    VerticalAlignment _VerticalAlignment;       // Vertical alignment of a visualization in the graph area.

    bool _FlipHorizontally;
    bool _FlipVertically;

    XAxisMode _XAxisMode;
    bool _XAxisTop;
    bool _XAxisBottom;
    uint32_t _XAxisDecimals;                    // Number of decimals to show on the x-axis labels, [0..3]

    YAxisMode _YAxisMode;
    bool _YAxisLeft;
    bool _YAxisRight;

    double _AmplitudeLo;                        // Lower amplitude, [-120, 0] dBFS
    double _AmplitudeHi;                        // Upper amplitude, [-120, 0] dBFS
    double _AmplitudeStep;

    bool _UseAbsolute;                          // Linear/n-th root scaling: Sets the min. dB range to -∞ dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
    double _Gamma;                              // Linear/n-th root scaling: Index n of the n-th root calculation, [0.5, 10.0]

    FLOAT _HRatio;
    FLOAT _VRatio;

    FLOAT _LPadding;
    FLOAT _RPadding;
    FLOAT _TPadding;
    FLOAT _BPadding;

    HorizontalTextAlignment _HorizontalTextAlignment;
    VerticalTextAlignment _VerticalTextAlignment;

    style_manager_t _StyleManager;

    static const uint32_t _CurrentVersion = 4;  // v0.10.0-alpha5
};

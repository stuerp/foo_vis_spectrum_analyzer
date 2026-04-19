
/** $VER: GraphDescription.h (2026.03.21) P. Stuer - Describes the layout and settings of a graph. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Constants.h"

#include <string>

#pragma warning(push)
#pragma warning(disable: 4868) // compiler may not enforce left-to-right evaluation order in braced initializer list

#include <nlohmann\json.hpp>

using json = nlohmann::ordered_json;

#pragma warning(pop)

/// <summary>
/// Represents the settings of a graph.
/// </summary>
#pragma warning(disable: 4820)
struct graph_description_t
{
public:
    graph_description_t()
    {
        Initialize();
    }

    graph_description_t(const std::wstring & description)
    {
        _Description = description;

        Initialize();
    }

    double ScaleAmplitude(double value) const noexcept;

    /* Code readability shortcuts */
    bool HasXAxis() const noexcept { return _XAxisMode != XAxisMode::None; }
    bool HasYAxis() const noexcept { return _YAxisMode != YAxisMode::None; }

    json ToJSON() const noexcept;
    static graph_description_t FromJSON(const json & object) noexcept;

private:
    void Initialize() noexcept;

public:
    std::wstring _Description;

    uint32_t _SelectedChannels;                 // The channels that should be visualized.

    uint32_t _ChannelPair;
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

    HorizontalTextAlignment _HAlignment;
    VerticalTextAlignment _VAlignment;

    static const uint32_t _CurrentVersion = 4;  // v0.10.0-alpha5
};

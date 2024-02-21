
/** $VER: GraphSettings.h (2024.02.21) P. Stuer - Represents the settings of a graph. **/

#pragma once

#include "framework.h"

#include "Constants.h"

/// <summary>
/// Represents the settings of a graph.
/// </summary>
struct GraphSettings
{
public:
    double ScaleA(double value) const;

public:
    std::wstring _Description;
    uint32_t _Channels;
    FLOAT _HRatio;
    FLOAT _VRatio;
    bool _FlipHorizontally;
    bool _FlipVertically;

    XAxisMode _XAxisMode;
    bool _XAxisTop;
    bool _XAxisBottom;

    YAxisMode _YAxisMode;
    bool _YAxisLeft;
    bool _YAxisRight;

    double _AmplitudeLo;    // Lower amplitude, -120.0 .. 0.0
    double _AmplitudeHi;    // Upper amplitude, -120.0 .. 0.0
    double _AmplitudeStep;

    bool _UseAbsolute;      // Linear/n-th root scaling: Sets the min. dB range to -∞ dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
    double _Gamma;          // Linear/n-th root scaling: Index n of the n-th root calculation, 0.5 .. 10.0
};

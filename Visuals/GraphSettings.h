
/** $VER: GraphSettings.h (2025.10.05) P. Stuer - Represents the settings of a graph. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

#include "Constants.h"

#include <string>

/// <summary>
/// Represents the settings of a graph.
/// </summary>
#pragma warning(disable: 4820)
struct graph_settings_t
{
public:
    graph_settings_t()
    {
        Initialize();
    }

    graph_settings_t(const std::wstring & description)
    {
        _Description = description;

        Initialize();
    }

    double ScaleA(double value) const;

private:
    void Initialize()
    {
        _SelectedChannels = (uint32_t) Channels::ConfigStereo;
        _ChannelPairs = (uint32_t) ChannelPair::FrontLeftRight;

        _HorizontalAlignment = HorizontalAlignment::Center;
        _VerticalAlignment = VerticalAlignment::Center;

        _FlipHorizontally = false;
        _FlipVertically = false;

        _XAxisMode = XAxisMode::Bands;
        _XAxisTop = false;
        _XAxisBottom = true;

        _YAxisMode = YAxisMode::Decibels;
        _YAxisLeft = true;
        _YAxisRight = false;

        _AmplitudeLo =  -90.;    // Lower amplitude, -120.0 .. 0.0
        _AmplitudeHi =    0.;    // Upper amplitude, -120.0 .. 0.0
        _AmplitudeStep = -6.;

        _UseAbsolute = true;    // Linear/n-th root scaling: Sets the min. dB range to -∞ dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
        _Gamma = 1.;            // Linear/n-th root scaling: Index n of the n-th root calculation, 0.5 .. 10.0

        _HRatio = 1.;
        _VRatio = 1.;

        _LPadding = 0.;
        _RPadding = 0.;
        _TPadding = 0.;
        _BPadding = 0.;

        _HAlignment = HorizontalTextAlignment::Center;
        _VAlignment = VerticalTextAlignment::Center;
    }

public:
    std::wstring _Description;

    uint32_t _SelectedChannels;                 // The channels that should be visualized.
    uint32_t _ChannelPairs;

    HorizontalAlignment _HorizontalAlignment;   // Horizonal alignment of a visualization in the graph area.
    VerticalAlignment _VerticalAlignment;       // Horizonal alignment of a visualization in the graph area.

    bool _FlipHorizontally;
    bool _FlipVertically;

    XAxisMode _XAxisMode;
    bool _XAxisTop;
    bool _XAxisBottom;

    YAxisMode _YAxisMode;
    bool _YAxisLeft;
    bool _YAxisRight;

    double _AmplitudeLo;                        // Lower amplitude, -120.0 .. 0.0 dBFS
    double _AmplitudeHi;                        // Upper amplitude, -120.0 .. 0.0 dBFS
    double _AmplitudeStep;

    bool _UseAbsolute;                          // Linear/n-th root scaling: Sets the min. dB range to -∞ dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
    double _Gamma;                              // Linear/n-th root scaling: Index n of the n-th root calculation, 0.5 .. 10.0

    FLOAT _HRatio;
    FLOAT _VRatio;

    FLOAT _LPadding;
    FLOAT _RPadding;
    FLOAT _TPadding;
    FLOAT _BPadding;

    HorizontalTextAlignment _HAlignment;
    VerticalTextAlignment _VAlignment;

    static const uint32_t _CurentVersion = 3;
};

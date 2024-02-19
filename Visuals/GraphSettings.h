
/** $VER: GraphSettings.h (2024.02.19) P. Stuer - Represents the settings of a graph. **/

#pragma once

#include "framework.h"

/// <summary>
/// Represents the settings of a graph.
/// </summary>
class GraphSettings
{
public:
    GraphSettings(const std::wstring & description, uint32_t channels, FLOAT hRatio, FLOAT vRatio, bool flipHorizontally, bool flipVertically) : _Description(description), _Channels(channels), _HRatio(hRatio), _VRatio(vRatio), _FlipHorizontally(flipHorizontally), _FlipVertically(flipVertically) { }

public:
    std::wstring _Description;
    uint32_t _Channels;
    FLOAT _HRatio;
    FLOAT _VRatio;
    bool _FlipHorizontally;
    bool _FlipVertically;
};

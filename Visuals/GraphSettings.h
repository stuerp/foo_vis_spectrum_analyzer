
/** $VER: GraphSettings.h (2024.02.18) P. Stuer - Represents the settings of a graph. **/

#pragma once

#include "framework.h"

/// <summary>
/// Represents the settings of a graph.
/// </summary>
class GraphSettings
{
public:
    GraphSettings(const std::wstring & description, uint32_t channels, bool flipHorizontally, bool flipVertically) : _Description(description), _Channels(channels), _FlipHorizontally(flipHorizontally), _FlipVertically(flipVertically) { }

public:
    std::wstring _Description;
    uint32_t _Channels;
    bool _FlipHorizontally;
    bool _FlipVertically;
};

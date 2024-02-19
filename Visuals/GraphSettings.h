
/** $VER: GraphSettings.h (2024.02.19) P. Stuer - Represents the settings of a graph. **/

#pragma once

#include "framework.h"

#include "Constants.h"

/// <summary>
/// Represents the settings of a graph.
/// </summary>
struct GraphSettings
{
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
};


/** $VER: Visualization.h (2026.09.14) P. Stuer - Base class for all visualization elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Element.h"

class visualization_t : public element_t
{
public:
    visualization_t() {}

    virtual ~visualization_t() noexcept {}

    virtual void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept = 0;

protected:
};

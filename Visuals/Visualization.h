
/** $VER: Visualization.h (2026.10.24) P. Stuer - Base class for all visualization elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Element.h"

#include <memory>

class visualization_t : public element_t
{
public:
    visualization_t(state_t & state) : element_t(state) { }

    virtual ~visualization_t() noexcept = default;

    virtual void Configure(graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept = 0;

    static std::unique_ptr<visualization_t> Create(state_t & state) noexcept;
};

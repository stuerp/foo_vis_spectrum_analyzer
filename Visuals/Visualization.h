
/** $VER: Visualization.h (2026.11.01) P. Stuer - Base class for all visualization elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Element.h"

#include <memory>

class visualization_t : public element_t
{
public:
    visualization_t(state_t & state, render_context_t & renderContext) : element_t(state, renderContext) { }

    virtual ~visualization_t() noexcept = default;

    virtual void Configure(graph_options_t * options, audio_processor_t * analysis, bool isFirst, bool isLast) noexcept = 0;
    virtual bool UsesDirect3D() const noexcept { return false; }

    static std::unique_ptr<visualization_t> Create(state_t & state, render_context_t & renderContext) noexcept;
};

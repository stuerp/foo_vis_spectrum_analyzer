
/** $VER: Visualization.h (2026.06.10) P. Stuer - Base class for all visualization elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Element.h"

class visualization_t : public element_t
{
public:
    visualization_t() {}

    virtual ~visualization_t() noexcept {}

    virtual void Initialize(state_t * state, graph_description_t * graphDescription, const analysis_t * analysis) noexcept = 0;

protected:
};


/** $VER: Element.h (2024.03.23) P. Stuer - Base class for all visual elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include "State.h"
#include "GraphSettings.h"

class Element
{
public:
    Element() : _State(), _GraphSettings() {}

    virtual ~Element() {}

    virtual const D2D1_RECT_F & GetBounds() const noexcept { return _Bounds; }

protected:
    State * _State;
    const GraphSettings * _GraphSettings;

    D2D1_RECT_F _Bounds;
    D2D1_SIZE_F _Size;
};

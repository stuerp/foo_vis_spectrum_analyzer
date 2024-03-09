
/** $VER: Element.h (2024.03.09) P. Stuer - Base class for all visual elements. **/

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

protected:
    State * _State;
    const GraphSettings * _GraphSettings;
};

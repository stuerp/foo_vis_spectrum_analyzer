
/** $VER: Element.h (2024.02.19) P. Stuer - Base class for all visual elements. **/

#pragma once

#include "framework.h"
#include "Support.h"

#include "State.h"
#include "GraphSettings.h"
#include "Style.h"

class Element
{
public:
    Element() : _State(), _GraphSettings() {}

protected:
    State * _State;
    const GraphSettings * _GraphSettings;
};

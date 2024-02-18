
/** $VER: Element.h (2024.02.18) P. Stuer - Base class for all visual elements. **/

#pragma once

#include "framework.h"
#include "Support.h"

#include "State.h"
#include "Style.h"

class Element
{
public:
    Element() : _State() {}

protected:
    State * _State;
    bool _FlipHorizontally;
    bool _FlipVertically;
};

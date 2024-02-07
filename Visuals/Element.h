
/** $VER: Element.h (2024.02.07) P. Stuer - Base class for all visual elements. **/

#pragma once

#include "framework.h"
#include "Support.h"

#include "Configuration.h"
#include "Style.h"

class Element
{
public:
    Element() : _Configuration() {}

protected:
    Configuration * _Configuration;
};

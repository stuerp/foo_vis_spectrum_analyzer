
/** $VER: Element.h (2024.02.05) P. Stuer - Base class for all visual elements. **/

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
    const Configuration * _Configuration;
};

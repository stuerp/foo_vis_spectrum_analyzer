
/** $VER: CUIElement.cpp (2023.12.10) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "CUIElement.h"

#pragma hdrstop

#include "ui_extension.h"

namespace uie
{
CUIElement::CUIElement()
{
    cui::colours::helper Helper(pfc::guid_null);

    _Configuration._DefBackColor = Helper.get_colour(cui::colours::colour_background);
    _Configuration._DefTextColor = Helper.get_colour(cui::colours::colour_text);
}

static window_factory<CUIElement> _WindowFactory;
}

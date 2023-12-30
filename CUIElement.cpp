
/** $VER: CUIElement.cpp (2023.12.30) P. Stuer **/

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

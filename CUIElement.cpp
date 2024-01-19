
/** $VER: CUIElement.cpp (2024.01.19) P. Stuer **/

#include "CUIElement.h"

#pragma hdrstop

#include "ui_extension.h"

namespace uie
{

/// <summary>
/// Initializes a new instance.
/// </summary>
CUIElement::CUIElement()
{
    cui::colours::helper Helper(pfc::guid_null);

    _Configuration._DefBackColor = Helper.get_colour(cui::colours::colour_background);
    _Configuration._DefTextColor = Helper.get_colour(cui::colours::colour_text);
}

/// <summary>
/// Creates or transfers the window.
/// </summary>
HWND CUIElement::create_or_transfer_window(HWND parent, const window_host_ptr & newHost, const ui_helpers::window_position_t & position)
{
    if ((HWND) *this)
    {
        ShowWindow(SW_HIDE);
        SetParent(parent);

        _Host->relinquish_ownership(*this);
        _Host = newHost;

        SetWindowPos(0, position.x, position.y, (int) position.cx, (int) position.cy, SWP_NOZORDER);
    }
    else
    {
        _Host = newHost;

        CRect r;

        position.convert_to_rect(r);

        Create(parent, r, 0, WS_CHILD, 0);
    }

    return *this;
}

/// <summary>
/// Destroys the window.
/// </summary>
void CUIElement::destroy_window()
{
    ::DestroyWindow(*this);

    _Host.release();
}

static window_factory<CUIElement> _WindowFactory;

}

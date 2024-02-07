
/** $VER: CUIElement.cpp (2024.02.04) P. Stuer **/

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
    _Configuration._IsDUI = false;

    cui::colours::helper Helper(pfc::guid_null);

    _Configuration._UserInterfaceColors.clear();

    _Configuration._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_text)));
    _Configuration._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_selection_text)));
    _Configuration._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_inactive_selection_text)));

    _Configuration._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_background)));
    _Configuration._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_selection_background)));
    _Configuration._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_inactive_selection_background)));

    _Configuration._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_active_item_frame)));
}

/// <summary>
/// Creates or transfers the window.
/// </summary>
HWND CUIElement::create_or_transfer_window(HWND hParent, const window_host_ptr & newHost, const ui_helpers::window_position_t & position)
{
    if (*this == nullptr)
    {
        _Host = newHost;

        CRect r;

        position.convert_to_rect(r);

        Create(hParent, r, 0, WS_CHILD, 0);

        _hParent = hParent;
    }
    else
    {
        ShowWindow(SW_HIDE);
        SetParent(hParent);

        _Host->relinquish_ownership(*this);
        _Host = newHost;

        SetWindowPos(NULL, position.x, position.y, (int) position.cx, (int) position.cy, SWP_NOZORDER);
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

/// <summary>
/// Toggles full screen mode.
/// </summary>
void CUIElement::ToggleFullScreen() noexcept
{
    _CriticalSection.Enter();

    LONG_PTR Style = ::GetWindowLongPtrW(m_hWnd, GWL_STYLE);

    if (!_IsFullScreen)
    {
        HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

        if (hMonitor != NULL)
        {
            MONITORINFOEXW mix = { { sizeof(mix) } };

            if (::GetMonitorInfoW(hMonitor, &mix))
            {
                ::SetWindowLongPtrW(m_hWnd, GWL_STYLE, (Style * (LONG_PTR) ~WS_CHILD) | (LONG_PTR) WS_POPUP);

                SetParent(::GetDesktopWindow());
                SetWindowPos(NULL, mix.rcWork.left, mix.rcWork.top, mix.rcWork.right - mix.rcWork.left, mix.rcWork.bottom - mix.rcWork.top, SWP_NOZORDER);

                _IsFullScreen = true;
            }
        }
    }
    else
    {
        RECT cr;

        ::GetClientRect(_hParent, &cr);

        ::SetWindowLongPtrW(m_hWnd, GWL_STYLE, (Style * (LONG_PTR) ~WS_POPUP) | (LONG_PTR) WS_CHILD);

        SetWindowPos(NULL, cr.left, cr.top, cr.right - cr.left, cr.bottom - cr.top, SWP_NOZORDER);
        SetParent(_hParent);

        _IsFullScreen = false;
    }

    _CriticalSection.Leave();
}

static window_factory<CUIElement> _WindowFactory;

}

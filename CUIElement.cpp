
/** $VER: CUIElement.cpp (2026.03.22) P. Stuer **/

#include "pch.h"

#include "CUIElement.h"
#include "CUINotificationHandler.h"

#include "Color.h"

#pragma hdrstop

namespace uie
{
/// <summary>
/// Initializes a new instance.
/// </summary>
cui_element_t::cui_element_t()
{
    _IsVisible = true; // CUI does send notifications.

    _UIState._IsDUI = false;

    GetColors();
}

/// <summary>
/// Destroys this instance.
/// </summary>
cui_element_t::~cui_element_t() noexcept
{
}

/// <summary>
/// Creates or transfers the window.
/// </summary>
HWND cui_element_t::create_or_transfer_window(HWND hParent, const window_host_ptr & newHost, const ui_helpers::window_position_t & position)
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

    cui_notification_handler_t::Register(this);

    return *this;
}

/// <summary>
/// Destroys the window.
/// </summary>
void cui_element_t::destroy_window()
{
    cui_notification_handler_t::Unregister(this);

    ::DestroyWindow(*this);

    _Host.release();
}
/*
/// <summary>
/// Handles the WM_ERASEBKGND message.
/// </summary>
LRESULT CUIElement::OnEraseBackground(CDCHandle hDC)
{
    if (!_IsInitializing)
        return 0;

    RECT cr;

    GetClientRect(&cr);

    HBRUSH hBrush = color_t::CreateBrush(_UIThread._StyleManager.UserInterfaceColors[3]);

    ::FillRect(hDC, &cr, hBrush);

    ::DeleteObject((HGDIOBJ) hBrush);

    _IsInitializing = false;

    return 1; // Prevent GDI from erasing the background. Required for transparency.
}
*/
/// <summary>
/// Toggles full screen mode.
/// </summary>
void cui_element_t::ToggleFullScreen() noexcept
{
    _CriticalSection.Enter();

    if (!_IsFullScreen)
    {
        HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

        if (hMonitor != NULL)
        {
            MONITORINFOEXW mix = { { sizeof(mix) } };

            if (::GetMonitorInfoW(hMonitor, &mix))
            {
                GetWindowRect(&_OldRect);

                CWindow(_hParent).ScreenToClient(&_OldRect);

                ShowWindow(SW_HIDE);
                SetParent(::GetDesktopWindow());

                LONG_PTR Style = ::GetWindowLongPtrW(m_hWnd, GWL_STYLE);

                ::SetWindowLongPtrW(m_hWnd, GWL_STYLE, (Style & (LONG_PTR) ~WS_CHILD) | (LONG_PTR) WS_POPUP);

                SetWindowPos(HWND_TOP, &mix.rcWork, SWP_ASYNCWINDOWPOS | SWP_NOZORDER | SWP_SHOWWINDOW);

                _Host->relinquish_ownership(nullptr);

                UpdateState(ConfigurationChanges::All);

                _IsFullScreen = true;
            }
        }
    }
    else
    {
        ShowWindow(SW_HIDE);
        SetParent(_hParent);

        LONG_PTR Style = ::GetWindowLongPtrW(m_hWnd, GWL_STYLE);

        ::SetWindowLongPtrW(m_hWnd, GWL_STYLE, (Style & (LONG_PTR) ~WS_POPUP) | (LONG_PTR) WS_CHILD);

        SetWindowPos(NULL, &_OldRect, SWP_ASYNCWINDOWPOS | SWP_NOZORDER | SWP_SHOWWINDOW);

        _Host->relinquish_ownership(_hParent);

        UpdateState(ConfigurationChanges::All);

        _IsFullScreen = false;
    }

    _CriticalSection.Leave();
}

/// <summary>
/// Gets the user interface colors.
/// </summary>
void cui_element_t::GetColors() noexcept
{
    cui::colours::helper Helper(pfc::guid_null);

    _UIState._StyleManager.UserInterfaceColors.clear();

    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(Helper.get_colour(cui::colours::colour_text)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(Helper.get_colour(cui::colours::colour_selection_text)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(Helper.get_colour(cui::colours::colour_inactive_selection_text)));

    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(Helper.get_colour(cui::colours::colour_background)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(Helper.get_colour(cui::colours::colour_selection_background)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(Helper.get_colour(cui::colours::colour_inactive_selection_background)));

    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(Helper.get_colour(cui::colours::colour_active_item_frame)));
}

static uie::window_factory<cui_element_t> _WindowFactory;
}

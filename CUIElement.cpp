
/** $VER: CUIElement.cpp (2024.03.12) P. Stuer **/

#include "CUIElement.h"
#include "Color.h"

#include "ui_extension.h"

#pragma hdrstop

namespace uie
{

/// <summary>
/// Initializes a new instance.
/// </summary>
CUIElement::CUIElement()
{
    _State._IsDUI = false;

    GetColors();

    _IsVisible = true; // CUI does send notifications.
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
/// Handles the WM_ERASEBKGND message.
/// </summary>
LRESULT CUIElement::OnEraseBackground(CDCHandle hDC)
{
    static bool IsStartup = true;

    if (!IsStartup)
        return 0;

    RECT cr;

    GetClientRect(&cr);

    HBRUSH hBrush = Color::CreateBrush(_State._StyleManager._UserInterfaceColors[3]);

    ::FillRect(hDC, &cr, hBrush);

    ::DeleteObject((HGDIOBJ) hBrush);

    IsStartup = false;

    return 1;
}

/// <summary>
/// Toggles full screen mode.
/// </summary>
void CUIElement::ToggleFullScreen() noexcept
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
                GetWindowRect(&_OldBounds);

                CWindow(_hParent).ScreenToClient(&_OldBounds);

                ShowWindow(SW_HIDE);
                SetParent(::GetDesktopWindow());

                LONG_PTR Style = ::GetWindowLongPtrW(m_hWnd, GWL_STYLE);

                ::SetWindowLongPtrW(m_hWnd, GWL_STYLE, (Style & (LONG_PTR) ~WS_CHILD) | (LONG_PTR) WS_POPUP);

                SetWindowPos(HWND_TOP, &mix.rcWork, SWP_ASYNCWINDOWPOS | SWP_NOZORDER | SWP_SHOWWINDOW);

                _Host->relinquish_ownership(nullptr);

                UpdateState();

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

        SetWindowPos(NULL, &_OldBounds, SWP_ASYNCWINDOWPOS | SWP_NOZORDER | SWP_SHOWWINDOW);

        _Host->relinquish_ownership(_hParent);

        UpdateState();

        _IsFullScreen = false;
    }

    _CriticalSection.Leave();
}

/// <summary>
/// Gets the user interface colors.
/// </summary>
void CUIElement::GetColors() noexcept
{
    cui::colours::helper Helper(pfc::guid_null);

    _State._StyleManager._UserInterfaceColors.clear();

    _State._StyleManager._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_text)));
    _State._StyleManager._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_selection_text)));
    _State._StyleManager._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_inactive_selection_text)));

    _State._StyleManager._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_background)));
    _State._StyleManager._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_selection_background)));
    _State._StyleManager._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_inactive_selection_background)));

    _State._StyleManager._UserInterfaceColors.push_back(D2D1::ColorF(Helper.get_colour(cui::colours::colour_active_item_frame)));
}

static window_factory<CUIElement> _WindowFactory;

}

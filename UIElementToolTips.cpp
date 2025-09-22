
/** $VER: ToolTips.cpp (2025.09.17) P. Stuer **/

#include "pch.h"
#include "UIElement.h"

#pragma hdrstop

/// <summary>
/// Creates the ToolTip control.
/// </summary>
void uielement_t::CreateToolTipControl() noexcept
{
    if (_ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE) == NULL)
        return;

    _ToolTipControl.SetMaxTipWidth(100);

    ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);
}

/// <summary>
/// Handles mouse move messages.
/// </summary>
void uielement_t::OnMouseMove(UINT, CPoint pt)
{
    if (!_ToolTipControl.IsWindow() || (!_RenderThread._ShowToolTipsAlways && !_RenderThread._ShowToolTipsNow))
        return;

    if (_TrackingGraph == nullptr)
    {
        _TrackingGraph = GetGraph(pt);

        if (_TrackingGraph == nullptr)
            return;

        // Tell Windows we want to know when the mouse leaves this window.
        {
            TRACKMOUSEEVENT tme =
            {
                .cbSize    = sizeof(TRACKMOUSEEVENT),
                .dwFlags   = TME_LEAVE,
                .hwndTrack = m_hWnd
            };
        
            ::TrackMouseEvent(&tme);
        }

        _TrackingGraph->InitToolInfo(m_hWnd, _TrackingToolInfo);

        _LastMousePos = pt;
        _LastBandIndex = ~0U;

        const FLOAT ScaledX = (FLOAT) ::MulDiv((int) pt.x, USER_DEFAULT_SCREEN_DPI, (int) _DPI);
        const FLOAT ScaledY = (FLOAT) ::MulDiv((int) pt.y, USER_DEFAULT_SCREEN_DPI, (int) _DPI);

        std::wstring Text;
        size_t BandIndex;

        if (_TrackingGraph->GetToolTipText(ScaledX, ScaledY, Text, BandIndex))
        {
            // Position the tooltip.
            {
                ::ClientToScreen(m_hWnd, &pt);

                RECT tr;

                _ToolTipControl.GetClientRect(&tr);

                int x = pt.x + 4;
                int y = pt.y - 4 - tr.bottom;

                RECT wr;

                GetWindowRect(&wr);

                if (x + tr.right >=  wr.right)
                    x = pt.x - 4 - tr.right;

                if (y <=  wr.top)
                    y = pt.y + 4;

                _ToolTipControl.TrackPosition(x, y);
            }

            _TrackingToolInfo.lpszText = (LPWSTR) Text.c_str();

            _ToolTipControl.UpdateTipText(&_TrackingToolInfo);
            _ToolTipControl.TrackActivate(&_TrackingToolInfo, TRUE);

            _LastBandIndex = BandIndex;
        }
    }
    else
    {
        if (pt == _LastMousePos)
            return;

        _LastMousePos = pt;

        const FLOAT ScaledX = (FLOAT) ::MulDiv((int) pt.x, USER_DEFAULT_SCREEN_DPI, (int) _DPI);
        const FLOAT ScaledY = (FLOAT) ::MulDiv((int) pt.y, USER_DEFAULT_SCREEN_DPI, (int) _DPI);

        std::wstring Text;
        size_t BandIndex;

        if (_TrackingGraph->GetToolTipText(ScaledX, ScaledY, Text, BandIndex))
        {
            // Position the tooltip.
            {
                ::ClientToScreen(m_hWnd, &pt);

                RECT tr;

                _ToolTipControl.GetClientRect(&tr);

                int x = pt.x + 4;
                int y = pt.y - 4 - tr.bottom;

                RECT wr;

                GetWindowRect(&wr);

                if (x + tr.right >=  wr.right)
                    x = pt.x - 4 - tr.right;

                if (y <=  wr.top)
                    y = pt.y + 4;

                _ToolTipControl.TrackPosition(x, y);
            }

            if (BandIndex != _LastBandIndex)
            {
                _TrackingToolInfo.lpszText = (LPWSTR) Text.c_str();

                _ToolTipControl.UpdateTipText(&_TrackingToolInfo);
                _ToolTipControl.TrackActivate(&_TrackingToolInfo, TRUE);

                _LastBandIndex = BandIndex;
            }
        }
        else
            DeleteTrackingToolTip();
    }
}

/// <summary>
/// Turns off the tracking tooltip when the mouse leaves the window.
/// </summary>
void uielement_t::OnMouseLeave()
{
    POINT pt; GetCursorPos(&pt); HWND hWndNew = WindowFromPoint(pt);

    if (hWndNew == _ToolTipControl.m_hWnd)
    {
        _TrackingGraph = nullptr; // Make sure we request future Mouse Leave notifications.

        return;
    }

    DeleteTrackingToolTip();
}

/// <summary>
/// Deletes the current tracking tooltip.
/// </summary>
void uielement_t::DeleteTrackingToolTip() noexcept
{
    if (!_ToolTipControl.IsWindow())
        return;

    _ToolTipControl.TrackActivate(&_TrackingToolInfo, FALSE);

    _TrackingGraph = nullptr;
    _LastBandIndex = ~0U;
}

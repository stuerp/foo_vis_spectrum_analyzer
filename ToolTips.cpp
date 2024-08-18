
/** $VER: ToolTips.cpp (2024.03.26) P. Stuer **/

#include "framework.h"
#include "UIElement.h"

#pragma hdrstop

/// <summary>
/// Creates the ToolTip control.
/// </summary>
void UIElement::CreateToolTipControl() noexcept
{
    if (_ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE) == NULL)
        return;

    _ToolTipControl.SetMaxTipWidth(100);

    ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);
}

/// <summary>
/// Deletes the current tracking tooltip.
/// </summary>
void UIElement::DeleteTrackingToolTip() noexcept
{
    if (!_ToolTipControl.IsWindow())
        return;

    _ToolTipControl.TrackActivate(&_TrackingToolInfo, FALSE);

    _TrackingGraph = nullptr;
}

/// <summary>
/// Handles mouse move messages.
/// </summary>
void UIElement::OnMouseMove(UINT, CPoint pt)
{
    if (!_ToolTipControl.IsWindow())
        return;

    if (_TrackingGraph == nullptr)
    {
        _TrackingGraph = GetGraph(pt);

        if (_TrackingGraph == nullptr)
            return;

        // Tell Windows we want to know when the mouse leaves this window.
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };

            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
        
            ::TrackMouseEvent(&tme);
        }

        _LastMousePos = pt;
        _LastIndex = ~0U;

        FLOAT ScaledX = (FLOAT) ::MulDiv((int) pt.x, USER_DEFAULT_SCREEN_DPI, (int) _DPI);
        FLOAT ScaledY = (FLOAT) ::MulDiv((int) pt.y, USER_DEFAULT_SCREEN_DPI, (int) _DPI);

        std::wstring Text;
        size_t Index;

        if (_TrackingGraph->GetToolTipText(ScaledX, ScaledY, Text, Index))
        {
            _TrackingGraph->InitToolInfo(m_hWnd, _TrackingToolInfo);

            _ToolTipControl.TrackActivate(&_TrackingToolInfo, TRUE);
        }
    }
    else
    {
        if (pt != _LastMousePos)
        {
            _LastMousePos = pt;

            FLOAT ScaledX = (FLOAT) ::MulDiv((int) pt.x, USER_DEFAULT_SCREEN_DPI, (int) _DPI);
            FLOAT ScaledY = (FLOAT) ::MulDiv((int) pt.y, USER_DEFAULT_SCREEN_DPI, (int) _DPI);

            std::wstring Text;
            size_t Index;

            if (_TrackingGraph->GetToolTipText(ScaledX, ScaledY, Text, Index))
            {
                if (Index != _LastIndex)
                {
                    _TrackingToolInfo.lpszText = (LPWSTR) Text.c_str();

                    _ToolTipControl.UpdateTipText(&_TrackingToolInfo);

                    _LastIndex = Index;
                }

                // Reposition the tooltip.
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
            else
            {
                _ToolTipControl.TrackActivate(&_TrackingToolInfo, FALSE);

                _TrackingGraph = nullptr;
            }
        }
    }
}

/// <summary>
/// Turns off the tracking tooltip when the mouse leaves the window.
/// </summary>
void UIElement::OnMouseLeave()
{
    DeleteTrackingToolTip();
}


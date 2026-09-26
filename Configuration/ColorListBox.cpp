
/** $VER: ColorListBox.cpp (2026.02.22) P. Stuer - Implements a list box that displays colors using WTL. **/

#include "pch.h"

#include "ColorListBox.h"
#include "ColorDialog.h"

#include "Theme.h"
#include "Color.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes the control.
/// </summary>
void color_list_box_t::Initialize(HWND hWnd) noexcept
{
    if (_IsSubclassed)
        return;

    ATLASSERT(::IsWindow(hWnd));

    __super::_hWnd = hWnd;

    _IsSubclassed = SubclassWindow(hWnd);

    if (!_IsSubclassed)
        return;

    CreateDeviceIndependentResources();
}

/// <summary>
/// Terminates the control.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void color_list_box_t::Terminate() noexcept
{
    if (!IsWindow() || !_IsSubclassed)
        return;

    DeleteDeviceSpecificResources();
    DeleteDeviceIndependentResources();

    UnsubclassWindow(TRUE);
    _IsSubclassed = false;
}

/// <summary>
/// Draws an item.
/// </summary>
void color_list_box_t::DrawItem(LPDRAWITEMSTRUCT dis) noexcept
{
    HDC hDC = dis->hDC;

    CRect ri = dis->rcItem;

    if (dis->itemID != (UINT) -1)
    {
        // Draw the background.
        {
            COLORREF Color = _Theme.GetSysColor((dis->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW);

            HPEN hPen = ::CreatePen(PS_SOLID, 1, Color);

            HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

            HBRUSH hBrush = ::CreateSolidBrush(Color);

            HGDIOBJ hOldBrush = ::SelectObject(hDC, hBrush);

            ::Rectangle(hDC, ri.left, ri.top, ri.right, ri.bottom);

            ::SelectObject(hDC, hOldBrush);

            ::DeleteObject(hBrush);

            ::SelectObject(hDC, hOldPen);

            ::DeleteObject(hPen);
        }

        // Draw the foreground.
        {
            COLORREF Color = _Theme.GetSysColor((dis->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);

            HPEN hPen = ::CreatePen(PS_SOLID, 1, Color);

            HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

            {
                HBRUSH hBrush = color_t::CreateBrush(_Colors[dis->itemID]);

                HGDIOBJ hOldBrush = ::SelectObject(hDC, hBrush);

                ::InflateRect(&ri, -2, -2);

                ::Rectangle(hDC, ri.left, ri.top, ri.right, ri.bottom);

                ::SelectObject(hDC, hOldBrush);

                ::DeleteObject(hBrush);
            }

            ::SelectObject(hDC, hOldPen);

            ::DeleteObject(hPen);
        }
    }

    if (dis->itemState & ODS_FOCUS)
        ::DrawFocusRect(hDC, &ri);
}

/// <summary>
/// Measures the size of an item.
/// </summary>
void color_list_box_t::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) noexcept
{
    lpMeasureItemStruct->itemHeight = 20;
}

/// <summary>
/// Gets the colors.
/// </summary>
void color_list_box_t::GetColors(std::vector<D2D1_COLOR_F> & colors) const noexcept
{
    colors = _Colors;
}

/// <summary>
/// Sets the colors.
/// </summary>
void color_list_box_t::SetColors(const std::vector<D2D1_COLOR_F> & colors) noexcept
{
    _Colors = colors;

    ResetContent();

    for (size_t Index = 0; Index < _Colors.size(); ++Index)
        AddString(nullptr);

    InvalidateRect(NULL);

    EnableWindow(colors.size() > 0);
}

/// <summary>
/// Handles a double-click on an item.
/// </summary>
LRESULT color_list_box_t::OnDblClick(WORD, WORD, HWND, BOOL & handled) noexcept
{
    const int Index = GetCurSel();

    if (Index == LB_ERR)
        return 0;

    D2D1_COLOR_F Color = _Colors[(size_t) Index];

    color_dialog_t cd;

    if (cd.SelectColor(m_hWnd, Color))
    {
        _Colors[(size_t) Index] = Color;

        SendChangedNotification();
    }

    return 0;
}

/// <summary>
/// Sends a notification to the parent that the content has changed.
/// </summary>
void color_list_box_t::SendChangedNotification() const noexcept
{
    const NMHDR nmhdr = { m_hWnd, (UINT_PTR) GetDlgCtrlID(), (UINT) NM_RETURN };

    ::SendMessageW(GetParent(), WM_NOTIFY, nmhdr.idFrom, (LPARAM) &nmhdr);
}

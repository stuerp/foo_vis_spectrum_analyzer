
/** $VER: CColorListBox.cpp (2025.10.19) P. Stuer - Implements a list box that displays colors using WTL. **/

#include "pch.h"
#include "CColorListBox.h"

#include "CColorDialogEx.h"

#include "Theme.h"
#include "Support.h"
#include "Color.h"

#pragma hdrstop

/// <summary>
/// Initializes the control.
/// </summary>
void CColorListBox::Initialize(HWND hWnd)
{
    ATLASSERT(::IsWindow(hWnd));

    __super::_hWnd = hWnd;

    SubclassWindow(hWnd);

    CreateDeviceIndependentResources();
}

/// <summary>
/// Terminates the control.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void CColorListBox::Terminate()
{
    if (!IsWindow())
        return;

    DeleteDeviceSpecificResources();
    DeleteDeviceIndependentResources();

    UnsubclassWindow(TRUE);
}

/// <summary>
/// Draws an item.
/// </summary>
void CColorListBox::DrawItem(LPDRAWITEMSTRUCT dis)
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
void CColorListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    lpMeasureItemStruct->itemHeight = 20;
}

/// <summary>
/// Gets the colors.
/// </summary>
void CColorListBox::GetColors(std::vector<D2D1_COLOR_F> & colors) const
{
    colors = _Colors;
}

/// <summary>
/// Sets the colors.
/// </summary>
void CColorListBox::SetColors(const std::vector<D2D1_COLOR_F> & colors)
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
LRESULT CColorListBox::OnDblClick(WORD, WORD, HWND, BOOL & handled)
{
    int Index = GetCurSel();

    if (Index == LB_ERR)
        return 0;

    D2D1_COLOR_F Color = _Colors[(size_t) Index];

    CColorDialogEx cd;

    if (cd.SelectColor(m_hWnd, Color))
    {
        _Colors[(size_t) Index] = Color;

        SendChangedNotification();
    }

    return 0;
}

/// <summary>
/// Sends a notification that the content has changed.
/// </summary>
void CColorListBox::SendChangedNotification() const noexcept
{
    NMHDR nmhdr = { m_hWnd, (UINT_PTR) GetDlgCtrlID(), (UINT) NM_RETURN };

    ::SendMessageW(GetParent(), WM_NOTIFY, nmhdr.idFrom, (LPARAM) &nmhdr);
}

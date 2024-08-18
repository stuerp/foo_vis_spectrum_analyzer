
/** $VER: CMenuListBox.cpp (2024.04.29) P. Stuer - Implements a list box acts like a menu using WTL. **/

#include "framework.h"
#include "CMenuListBox.h"

#include "Theme.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Initializes the control.
/// </summary>
void CMenuListBox::Initialize(HWND hWnd)
{
    ATLASSERT(::IsWindow(hWnd));

    SubclassWindow(hWnd);
}

/// <summary>
/// Terminates the control.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void CMenuListBox::Terminate()
{
    if (!IsWindow())
        return;

    UnsubclassWindow(TRUE);
}

/// <summary>
/// Draws an item.
/// </summary>
void CMenuListBox::DrawItem(LPDRAWITEMSTRUCT dis)
{
    HDC hDC = dis->hDC;

    CRect ri = dis->rcItem;

    // Draw the background.
    {
        COLORREF Color = _Theme.GetSysColor((dis->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW);

        HPEN hPen = ::CreatePen(PS_SOLID, 1, Color);

        HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

        HBRUSH hBrush = ::CreateSolidBrush(Color);

        HGDIOBJ hOldBrush = ::SelectObject(hDC, hBrush);

        ::Rectangle(hDC, ri.left, ri.top, ri.right, ri.bottom);

        if (dis->itemState & ODS_FOCUS)
            ::DrawFocusRect(hDC, &ri);

        ::SelectObject(hDC, hOldBrush);

        ::DeleteObject(hBrush);

        ::SelectObject(hDC, hOldPen);

        ::DeleteObject(hPen);
    }

    // Draw the foreground.
    {
        COLORREF Color = _Theme.GetSysColor((dis->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);

        COLORREF OldColor = ::SetTextColor(hDC, Color);

        int OldBkMode = ::SetBkMode(hDC, TRANSPARENT);

        {
            CString Text;

            this->GetText((int) dis->itemID, Text);

            ::InflateRect(&ri, -4, -4);

            ::DrawTextW(hDC, Text, Text.GetLength(), &ri, DT_SINGLELINE | DT_VCENTER);
        }

        ::SetBkMode(hDC, OldBkMode);

        ::SetTextColor(hDC, OldColor);
    }
}

/// <summary>
/// Measures the size of an item.
/// </summary>
void CMenuListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    lpMeasureItemStruct->itemHeight = 32;
}

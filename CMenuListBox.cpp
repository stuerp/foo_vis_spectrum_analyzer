
/** $VER: CMenuListBox.cpp (2023.12.11) P. Stuer - Implements a list box acts like a menu using WTL. **/

#include "CMenuListBox.h"

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

    {
        HPEN hPen = ::CreatePen(PS_SOLID, 1, ::GetSysColor((dis->itemState & ODS_FOCUS) ? COLOR_HIGHLIGHT : COLOR_WINDOW));

        HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

        if (dis->itemState & (ODS_FOCUS | ODS_SELECTED))
            ::SelectObject(hDC, ::GetSysColorBrush(COLOR_HIGHLIGHT));
        else
            ::SelectObject(hDC, ::GetSysColorBrush(COLOR_WINDOW));

        ::Rectangle(hDC, ri.left, ri.top, ri.right, ri.bottom);

        if (dis->itemState & ODS_FOCUS)
            ::DrawFocusRect(hDC, &ri);

        ::SelectObject(hDC, hOldPen);

        ::DeleteObject(hPen);
    }
    {
        HPEN hPen = ::CreatePen(PS_SOLID, 1, ::GetSysColor((dis->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

        HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

        int OldBkMode = ::SetBkMode(hDC, TRANSPARENT);

        CString Text;

        this->GetText((int) dis->itemID, Text);

        ::InflateRect(&ri, -4, -4);

        ::DrawTextW(hDC, Text, Text.GetLength(), &ri, DT_SINGLELINE | DT_VCENTER);

        ::SetBkMode(hDC, OldBkMode);

        ::SelectObject(hDC, hOldPen);

        ::DeleteObject(hPen);
    }
}

/// <summary>
/// Measures the size of an item.
/// </summary>
void CMenuListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    lpMeasureItemStruct->itemHeight = 32;
}

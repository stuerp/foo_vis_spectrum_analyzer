
/** $VER: CColorListBox.cpp (2023.11.25) P. Stuer - Implements a list box that displays colors using WTL. **/

#include "framework.h"

#include "CColorListBox.h"

void CColorListBox::Initialize(HWND hWnd)
{
    ATLASSERT(::IsWindow(hWnd));

    SubclassWindow(hWnd);

    CreateDeviceIndependentResources();
}

/// <summary>
/// Adds a color after the selected item.
/// </summary>
bool CColorListBox::Add(const D2D1_COLOR_F & color)
{
    int Index = GetCurSel();

    if (Index == LB_ERR)
        return false;

    _Colors.insert(_Colors.begin() + Index + 1, color);

    UpdateItems();

    SetCurSel(Index);

    return true;
}

/// <summary>
/// Removes the selected item.
/// </summary>
bool CColorListBox::Remove()
{
    int Index = GetCurSel();

    if (Index == LB_ERR)
        return false;

    _Colors.erase(_Colors.begin() + Index);

    UpdateItems();

    SetCurSel(Index);

    return true;
}

/// <summary>
/// Draws an item.
/// </summary>
void CColorListBox::DrawItem(LPDRAWITEMSTRUCT dis)
{
    HDC hDC = dis->hDC;

    CRect ri = dis->rcItem;

    {
        HPEN hPen = ::CreatePen(PS_SOLID, 1, ::GetSysColor((dis->itemState & ODS_FOCUS) ? COLOR_HIGHLIGHT : COLOR_WINDOW));

        HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

        if (dis->itemState & ODS_FOCUS)
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
        HPEN hPen = ::CreatePen(PS_SOLID, 1, ::GetSysColor((dis->itemState & ODS_FOCUS) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

        HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

        {
            HBRUSH hBrush = ::CreateSolidBrush(ToCOLORREF(_Colors[dis->itemID]));

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

    UpdateItems();
}

/// <summary>
/// Handles a double-click on an item.
/// </summary>
LRESULT CColorListBox::OnDblClick(WORD, WORD, HWND, BOOL & handled)
{
    int Index = GetCurSel();

    if (Index == LB_ERR)
        return 0;

    D2D1_COLOR_F Color = _Colors[Index];

    if (SelectColor(m_hWnd, Color))
    {
        _Colors[Index] = Color;

        SendColorsChangedNotification();
    }

    return 0;
}

/// <summary>
/// Updates the list box items.
/// </summary>
void CColorListBox::UpdateItems()
{
    ResetContent();

    for (size_t Index = 0; Index < _Colors.size(); ++Index)
        AddString(nullptr);

    SendColorsChangedNotification();

    InvalidateRect(NULL);
}

/// <summary>
/// Sends a notification that the gradient has changed.
/// </summary>
void CColorListBox::SendColorsChangedNotification() const noexcept
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = m_hWnd;
    nmhdr.idFrom = GetDlgCtrlID();
    nmhdr.code = (UINT) NM_RETURN;

    ::SendMessageW(GetParent(), WM_NOTIFY, nmhdr.idFrom, (LPARAM) &nmhdr);
}

#pragma region DirectX

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT CColorListBox::CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size)
{
    HRESULT hr = __super::CreateDeviceSpecificResources(hWnd, size);

    if (SUCCEEDED(hr) && (_SolidBrush == nullptr))
        hr = _RenderTarget->CreateSolidColorBrush(_Color, &_SolidBrush);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void CColorListBox::ReleaseDeviceSpecificResources()
{
    _SolidBrush.Release();

    __super::ReleaseDeviceSpecificResources();
}

#pragma endregion


/** $VER: CColorListBox.cpp (2024.02.04) P. Stuer - Implements a list box that displays colors using WTL. **/

#include "framework.h"
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

    ReleaseDeviceSpecificResources();
    ReleaseDeviceIndependentResources();

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
                HBRUSH hBrush = Color::CreateBrush(_Colors[dis->itemID]);

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
/*
#pragma region DirectX

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT CColorListBox::CreateDeviceSpecificResources()
{
    HRESULT hr = __super::CreateDeviceSpecificResources();

    if (SUCCEEDED(hr) && (_SolidBrush == nullptr))
        hr = _RenderTarget->CreateSolidColorBrush(D2D1_COLOR_F(), &_SolidBrush);

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
*/
#pragma endregion

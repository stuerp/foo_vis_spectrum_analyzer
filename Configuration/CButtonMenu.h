
/** $VER: CButtonMenu.h (2024.01.21) P. Stuer - Based on Menu Button control (https://www.viksoe.dk/code/buttonmenu.htm) by Bjarke Viksoe **/

#pragma once

#include "framework.h"

#include "Color.h"
#include "Theme.h"

#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#define BMS_EX_CHECKMARKS               0x00000001
#define BMS_EX_FREESIZE                 0x00000002
#define BMS_EX_SHAREDIMAGELIST          0x00000004
#define BMS_EX_ICONS_AFTER_CHECKMARKS   0x00000008
#define BMS_EX_HOTCHECKLINE             0x00000010

#define GLYPH_ARROW_DOWN    L"\xE011"
#define GLYPH_CHECKMARK     L"\xE001"

template <class T, class TBase = CButton, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CButtonMenuImpl : public CWindowImpl<T, TBase, TWinTraits>, public CCustomDraw<T>
{
public:
    CButtonMenuImpl() : _IsMenuOwner(FALSE), _MenuStyle() { }

    virtual ~CButtonMenuImpl()
    {
        if (_IsMenuOwner)
            _Menu.DestroyMenu();

        if ((_MenuStyle & BMS_EX_SHAREDIMAGELIST) == 0)
            _ImageList.Destroy();
    }

    /// <summary>
    /// 
    /// </summary>
    DWORD OnPrePaint(int, LPNMCUSTOMDRAW)
    {
        return CDRF_NOTIFYPOSTPAINT;
    }

    /// <summary>
    /// 
    /// </summary>
    DWORD OnPostPaint(int, LPNMCUSTOMDRAW nmcd)
    {
        CDCHandle dc = nmcd->hdc;
        RECT& rc = nmcd->rc;

        // Draw the vertical separator.
        {
            RECT r = { rc.right - 22, rc.top + 6, rc.right - 20, rc.bottom - 6 };

            dc.Draw3dRect(&r, _Theme.GetSysColor(COLOR_BTNSHADOW), _Theme.GetSysColor(COLOR_BTNHIGHLIGHT));
        }

        // Draw the arrow.
        {
            bool IsDisabled = (nmcd->uItemState & (CDIS_DISABLED | CDIS_GRAYED)) != 0;

            dc.SetBkMode(TRANSPARENT);
            dc.SetTextColor(_Theme.GetSysColor(IsDisabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT));

            HFONT hOldFont = dc.SelectFont(_Assets);

            RECT r = { rc.right - 18, rc.top, rc.right, rc.bottom };

            dc.DrawText(GLYPH_ARROW_DOWN, 1, &r, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            dc.SelectFont(hOldFont);
        }

        return CDRF_DODEFAULT;
    }

    CMenuHandle & GetMenu()
    {
        return _Menu;
    }

    void SetMenu(UINT id)
    {
        if (_IsMenuOwner)
            _Menu.DestroyMenu();

        _Menu.LoadMenu(id);

        _IsMenuOwner = true;
    }

    void SetMenu(HMENU hMenu)
    {
        if (_IsMenuOwner)
            _Menu.DestroyMenu();

        _Menu.Attach(hMenu);

        _IsMenuOwner = false;
    }

    void SetExtendedMenuStyle(DWORD menuStyle)
    {
        _MenuStyle = menuStyle;
    }

    void SetImageList(HIMAGELIST hImageList)
    {
        _ImageList.Attach(hImageList);
    }

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    BOOL Initialize(HWND hWnd)
    {
        ATLASSERT(this->m_hWnd == NULL);
        ATLASSERT(::IsWindow(hWnd));

        BOOL Success = CWindowImpl<T, TBase, TWinTraits>::SubclassWindow(hWnd);

        if (Success)
            InternalInitialize();

        return Success;
    }

    /// <summary>
    /// Terminates this instance.
    /// </summary>
    void Terminate()
    {
        if (!IsWindow(this->m_hWnd))
            return;

        CWindowImpl<T, TBase, TWinTraits>::UnsubclassWindow(TRUE);
    }

    /// <summary>
    /// 
    /// </summary>
    LRESULT OnClicked(WORD, WORD, HWND, BOOL &)
    {
        ATLASSERT(_Menu.IsMenu());

        CMenuHandle Menu = PrepareMenu();

        RECT wr = { };

        this->GetWindowRect(&wr);

        POINT Position = { wr.left + 2, wr.bottom - 2 };

        CWindow w = this->GetTopLevelWindow();

        w.SendMessage(WM_INITMENUPOPUP, (WPARAM) (HMENU) Menu);

        TPMPARAMS tpmp = { };

        tpmp.cbSize = sizeof(TPMPARAMS);
        tpmp.rcExclude = wr;

        ::InflateRect(&tpmp.rcExclude, -1, -1);

        UINT Id = (UINT) ::TrackPopupMenuEx(Menu, TPM_RETURNCMD | TPM_VERPOSANIMATION | TPM_VERTICAL, Position.x, Position.y, this->m_hWnd, &tpmp);

        if (Id == 0)
            return 0;

        ::PostMessageW(this->GetParent(), WM_COMMAND, MAKEWPARAM(Id, 0), 0);

        return 0;
    }

private:
    enum
    {
        CX_MENUSPACE =   3,
        CY_MENUSPACE =   2,

        CX_ICONSPACE =   2,
        CX_CHECKSIZE =  16,
        CX_CHECKSPACE =  2,
    };

    /// <summary>
    /// 
    /// </summary>
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL &)
    {
        LRESULT Result = this->DefWindowProcW();

        InternalInitialize();

        return Result;
    }

    /// <summary>
    /// 
    /// </summary>
    void InternalInitialize()
    {
        ATLASSERT(::IsWindow(this->m_hWnd));

        this->SetButtonStyle(BS_PUSHBUTTON);

        CLogFont lf = AtlGetDefaultGuiFont();

        ::wcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), L"Segoe MDL2 Assets");

        lf.lfHeight = -::MulDiv(825, 96, 72) / 100; // 8.25pt

        if (_Assets.IsNull())
            _Assets.CreateFontIndirect(&lf);

        lf.SetMenuFont();

        if (_MenuFont.IsNull())
            _MenuFont.CreateFontIndirect(&lf);
    }

    /// <summary>
    /// 
    /// </summary>
    LRESULT OnKeyUp(UINT, WPARAM, LPARAM, BOOL & handled)
    {
    //      if (wParam == VK_F4)
    //          Click();

        handled = FALSE;

        return 0;
    }

    /// <summary>
    /// 
    /// </summary>
    void OnMeasureItem(UINT, LPMEASUREITEMSTRUCT mis)
    {
        mis->itemWidth  = (UINT) _Size.cx;
        mis->itemHeight = (UINT) _Size.cy;
    }

    /// <summary>
    /// 
    /// </summary>
    void OnDrawItem(UINT, LPDRAWITEMSTRUCT dis)
    {
        CDCHandle dc = dis->hDC;
        RECT& rc = dis->rcItem;

        UINT Index = (UINT) dis->itemData;

        TCHAR Text[200] = { };

        CMenuItemInfo mii;

        {
            mii.fMask = MIIM_TYPE | MIIM_STATE;
            mii.dwTypeData = Text;
            mii.cch = _countof(Text);

            _Menu.GetSubMenu(0).GetMenuItemInfoW(Index, TRUE, &mii);
        }

        bool IsSeparator = (mii.fType & MFT_SEPARATOR) != 0;

        COLORREF BackColor = _Theme.GetSysColor(COLOR_MENU);
        COLORREF TextColor = _Theme.GetSysColor(COLOR_MENUTEXT);

        COLORREF HighlightColor = _Theme.GetSysColor(COLOR_HIGHLIGHT);

        // Draw the background.
        {
            if ((_MenuStyle & BMS_EX_HOTCHECKLINE) != 0 && (dis->itemState & ODS_CHECKED) != 0)
            {
                BackColor = Color::Blend(HighlightColor, BackColor, 80);
            }

            if (((dis->itemState & ODS_SELECTED) != 0) && !IsSeparator)
            {
                BackColor = HighlightColor;
                TextColor = _Theme.GetSysColor(COLOR_HIGHLIGHTTEXT);
            }

            if ((dis->itemState & ODS_DISABLED) != 0)
                TextColor = _Theme.GetSysColor(COLOR_GRAYTEXT);

            dc.FillSolidRect(&rc, BackColor);
        }

        ::InflateRect(&rc, -CX_MENUSPACE, -CY_MENUSPACE);

        if (!IsSeparator)
        {
            dc.SetBkMode(TRANSPARENT);
            dc.SetTextColor(TextColor);

            // Draw the checkmark.
            {
                if ((_MenuStyle & (BMS_EX_CHECKMARKS | BMS_EX_ICONS_AFTER_CHECKMARKS)) == (BMS_EX_CHECKMARKS | BMS_EX_ICONS_AFTER_CHECKMARKS))
                {
                    if ((dis->itemState & ODS_CHECKED) != 0)
                    {
                        HFONT hOldFont = dc.SelectFont(_Assets);

                        dc.DrawText(GLYPH_CHECKMARK, 1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

                        dc.SelectFont(hOldFont);
                    }

                    rc.left += CX_CHECKSIZE + CX_CHECKSPACE;
                }
            }

            // Draw the icon.
            if (!_ImageList.IsNull())
            {
                SIZE Size = { };

                _ImageList.GetIconSize(Size);

                CIcon Icon = _ImageList.GetIcon((int) Index, ILD_TRANSPARENT);
                POINT pt = { rc.left, rc.top + ((rc.bottom - rc.top) / 2) - (Size.cy / 2) };

                dc.DrawIconEx(pt.x, pt.y, Icon, Size.cx, Size.cy);

                rc.left += Size.cx + CX_ICONSPACE;
            }

            // Draw the checkmark.
            if ((_MenuStyle & (BMS_EX_CHECKMARKS | BMS_EX_ICONS_AFTER_CHECKMARKS)) == (BMS_EX_CHECKMARKS))
            {
                if ((dis->itemState & ODS_CHECKED) != 0)
                {
                    HFONT hOldFont = dc.SelectFont(_Assets);

                    dc.DrawText(GLYPH_CHECKMARK, 1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

                    dc.SelectFont(hOldFont);
                }

                rc.left += CX_CHECKSIZE + CX_CHECKSPACE;
            }

            // Draw the text.
            {
                HFONT hOldFont = dc.SelectFont(_MenuFont);

                dc.DrawText(Text, -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

                dc.SelectFont(hOldFont);
            }
        }
        else
        {
            LONG y = rc.top + ((rc.bottom - rc.top) / 2);
            RECT r = { rc.left, y - 1, rc.right, y + 1 };

            dc.Draw3dRect(&r, _Theme.GetSysColor(COLOR_BTNSHADOW), _Theme.GetSysColor(COLOR_BTNHIGHLIGHT));
        }
    }

    /// <summary>
    /// Prepares the menu.
    /// </summary>
    CMenuHandle PrepareMenu()
    {
        _Size = { };

        CMenu Dst;

        Dst.CreatePopupMenu();

        CClientDC dc = this->m_hWnd;

        SIZE IconSize = { };

        if (!_ImageList.IsNull())
            _ImageList.GetIconSize(IconSize);

        {
            HFONT hOldFont = dc.SelectFont(_MenuFont);

            TEXTMETRIC tm = { };

            dc.GetTextMetrics(&tm);

            _Size.cy = max(IconSize.cy, tm.tmHeight);

            {
                CMenuHandle Src = _Menu.GetSubMenu(0);

                UINT ItemCount = (UINT) Src.GetMenuItemCount();

                for (UINT i = 0; i < ItemCount; ++i)
                {
                    WCHAR Text[200] = { };

                    CMenuItemInfo mii;

                    {
                        mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
                        mii.dwTypeData = Text;
                        mii.cch = _countof(Text);

                        Src.GetMenuItemInfo(i, TRUE, &mii);
                    }

                    {
                        mii.fState = ((mii.fState & MF_CHECKED) != 0U) ? MF_CHECKED : 0U;;
                        mii.fType = MF_OWNERDRAW;
                        mii.dwItemData = i;

                        Dst.InsertMenuItem(i, TRUE, &mii);
                    }

                    {
                        SIZE TextExtent = { };

                        dc.GetTextExtent(Text, -1, &TextExtent);

                        _Size.cx = max(_Size.cx, TextExtent.cx + 4);
                    }
                }
            }

            dc.SelectFont(hOldFont);
        }

        _Size.cy += CX_MENUSPACE * 2;
        _Size.cx += CY_MENUSPACE * 2;

        if (IconSize.cx > 0)
            _Size.cx += IconSize.cx + CX_ICONSPACE;

        if ((_MenuStyle & BMS_EX_CHECKMARKS) != 0)
            _Size.cx += CX_CHECKSIZE + CX_CHECKSPACE;

        if ((_MenuStyle & BMS_EX_FREESIZE) == 0)
        {
            RECT rw = { };

            this->GetWindowRect(&rw);

            _Size.cx = max(_Size.cx, rw.right - rw.left);

            _Size.cx -= ::GetSystemMetrics(SM_CYMENUCHECK);
        }

        return Dst.Detach();
    }

    BEGIN_MSG_MAP(CButtonMenuImpl)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)

        MSG_WM_MEASUREITEM(OnMeasureItem)
        MSG_WM_DRAWITEM(OnDrawItem)

        REFLECTED_COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)

        CHAIN_MSG_MAP_ALT(CCustomDraw<T>, 1)
    END_MSG_MAP()

private:
    CMenuHandle _Menu;
    bool _IsMenuOwner;
    DWORD _MenuStyle;

    CFont _Assets;
    CFont _MenuFont;
    CImageList _ImageList;
    SIZE _Size;
};

class CButtonMenu : public CButtonMenuImpl<CButtonMenu, CButton, CWinTraitsOR<BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_TABSTOP>>
{
public:
    DECLARE_WND_SUPERCLASS(_T("CButtonMenu"), GetWndClassName())
};

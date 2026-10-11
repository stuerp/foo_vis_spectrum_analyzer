
/** $VER: MenuListBox.h (2026.02.21) P. Stuer - Implements a list box acts like a menu using WTL. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "OwnerDrawnListBox.h"

class menu_list_box_t : public owner_drawn_listbox_t<menu_list_box_t>
{
public:
    menu_list_box_t() = default;

    menu_list_box_t(const menu_list_box_t &) = delete;
    menu_list_box_t & operator=(const menu_list_box_t &) = delete;
    menu_list_box_t(menu_list_box_t &&) = delete;
    menu_list_box_t & operator=(menu_list_box_t &&) = delete;

    virtual ~menu_list_box_t() = default;

    void Initialize(HWND hWnd) noexcept;
    void Terminate() noexcept;

    void DrawItem(LPDRAWITEMSTRUCT dis) noexcept;
    void MeasureItem(LPMEASUREITEMSTRUCT mis) noexcept;

    BEGIN_MSG_MAP(menu_list_box_t)
        CHAIN_MSG_MAP(owner_drawn_listbox_t<menu_list_box_t>)
    END_MSG_MAP()

private:
    bool _IsSubclassed { false };
};

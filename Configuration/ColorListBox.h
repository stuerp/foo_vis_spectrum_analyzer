
/** $VER: color_list_box_t.h (2026.02.21) P. Stuer - Implements a list box that displays colors using WTL. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <atlbase.h>
#include <atltypes.h>
#include <atlstr.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>
#include <atlcom.h>
#include <atlcrack.h>

#include <vector>

#include "DirectXControl.h"
#include "OwnerDrawnListbox.h"

class color_list_box_t : public owner_drawn_listbox_t<color_list_box_t>, public directx_control_t
{
public:
    color_list_box_t() { }

    color_list_box_t(const color_list_box_t &) = delete;
    color_list_box_t & operator=(const color_list_box_t &) = delete;
    color_list_box_t(color_list_box_t &&) = delete;
    color_list_box_t & operator=(color_list_box_t &&) = delete;

    virtual ~color_list_box_t() noexcept { }

    void Initialize(HWND hWnd) noexcept;
    void Terminate() noexcept;

    void GetColors(std::vector<D2D1_COLOR_F> & colors) const noexcept;
    void SetColors(const std::vector<D2D1_COLOR_F> & colors) noexcept;

    void DrawItem(LPDRAWITEMSTRUCT dis) noexcept;
    void MeasureItem(LPMEASUREITEMSTRUCT mis) noexcept;

private:
    LRESULT OnDblClick(WORD, WORD, HWND, BOOL & handled) noexcept;

    void SendChangedNotification() const noexcept;

    BEGIN_MSG_MAP(color_list_box_t)
        MSG_WM_SIZE(OnSize)

        REFLECTED_COMMAND_CODE_HANDLER(LBN_DBLCLK, OnDblClick)

        CHAIN_MSG_MAP(owner_drawn_listbox_t<color_list_box_t>)
    END_MSG_MAP()

private:
    std::vector<D2D1_COLOR_F> _Colors;
};

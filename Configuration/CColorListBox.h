
/** $VER: CColorListBox.h (2024.03.09) P. Stuer - Implements a list box that displays colors using WTL. **/

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

#include "CDirectXControl.h"
#include "COwnerDrawnListBox.h"
#include "Support.h"

class CColorListBox : public COwnerDrawnListBox<CColorListBox>, public CDirectXControl
{
public:
    CColorListBox() { }

    CColorListBox(const CColorListBox &) = delete;
    CColorListBox & operator=(const CColorListBox &) = delete;
    CColorListBox(CColorListBox &&) = delete;
    CColorListBox & operator=(CColorListBox &&) = delete;

    virtual ~CColorListBox() { }

    void Initialize(HWND hWnd);
    void Terminate();

    void GetColors(std::vector<D2D1_COLOR_F> & colors) const;
    void SetColors(const std::vector<D2D1_COLOR_F> & colors);

    void DrawItem(LPDRAWITEMSTRUCT dis);
    void MeasureItem(LPMEASUREITEMSTRUCT mis);

private:
    LRESULT OnDblClick(WORD, WORD, HWND, BOOL & handled);

    void SendChangedNotification() const noexcept;

    BEGIN_MSG_MAP(CColorListBox)
        MSG_WM_SIZE(OnSize)

        REFLECTED_COMMAND_CODE_HANDLER(LBN_DBLCLK, OnDblClick)

        CHAIN_MSG_MAP(COwnerDrawnListBox<CColorListBox>)
    END_MSG_MAP()

private:
    std::vector<D2D1_COLOR_F> _Colors;
};

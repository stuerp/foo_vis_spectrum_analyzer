
/** $VER: CMenuListBox.h (2024.03.09) P. Stuer - Implements a list box acts like a menu using WTL. **/

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

#include "CDirectXControl.h"
#include "COwnerDrawnListBox.h"

class CMenuListBox : public COwnerDrawnListBox<CMenuListBox>
{
public:
    CMenuListBox() { }

    CMenuListBox(const CMenuListBox &) = delete;
    CMenuListBox & operator=(const CMenuListBox &) = delete;
    CMenuListBox(CMenuListBox &&) = delete;
    CMenuListBox & operator=(CMenuListBox &&) = delete;

    virtual ~CMenuListBox() { }

    void Initialize(HWND hWnd);
    void Terminate();

    void DrawItem(LPDRAWITEMSTRUCT dis);
    void MeasureItem(LPMEASUREITEMSTRUCT mis);

    BEGIN_MSG_MAP(CMenuListBox)
        CHAIN_MSG_MAP(COwnerDrawnListBox<CMenuListBox>)
    END_MSG_MAP()

private:
    bool _DarkMode;
};

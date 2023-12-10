
/** $VER: CMenuListBox.h (2023.12.09) P. Stuer - Implements a list box acts like a menu using WTL. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "CDirectXControl.h"
#include "COwnerDrawnListBox.h"
#include "Support.h"

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
        REFLECTED_COMMAND_CODE_HANDLER(LBN_DBLCLK, OnDblClick)

        CHAIN_MSG_MAP(COwnerDrawnListBox<CMenuListBox>)
    END_MSG_MAP()

private:
    LRESULT OnSelectionChanged(WORD, WORD, HWND, BOOL & handled);

    LRESULT OnDblClick(WORD, WORD, HWND, BOOL & handled);

    void SendChangedNotification() const noexcept;
};

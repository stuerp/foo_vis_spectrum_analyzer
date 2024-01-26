
/** $VER: CMenuListBox.h (2024.01.21) P. Stuer - Implements a list box acts like a menu using WTL. **/

#pragma once

#include "framework.h"
#include "Support.h"

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

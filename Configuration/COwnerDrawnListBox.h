
/** $VER: COwnerDrawnListBox.h (2023.11.22) P. Stuer - Implements an owner-drawn list box using WTL. **/

#pragma once

#include "framework.h"

#include <atlapp.h>     // Base WTL classes
#include <atlframe.h>

template <class TBase>
class ATL_NO_VTABLE COwnerDrawnListBox : public CWindowImpl<TBase, CListBox>, public COwnerDraw<TBase>
{
public:
    BEGIN_MSG_MAP(COwnerDrawnListBox)
        CHAIN_MSG_MAP_ALT(COwnerDraw<TBase>, 1)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()
};

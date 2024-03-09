
/** $VER: COwnerDrawnListBox.h (2024.03.09) P. Stuer - Implements an owner-drawn list box using WTL. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

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

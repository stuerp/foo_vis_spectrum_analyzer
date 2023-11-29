
/** $VER: CNumericEdit.h (2023.11.29) P. Stuer - Implements a numeric edit box using WTL. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

class CNumericEdit: public CWindowImpl<CNumericEdit, CEdit>
{
public:
    BEGIN_MSG_MAP(CNumericEdit)
        MESSAGE_HANDLER(EM_SETSEL, OnSetSel)
    END_MSG_MAP()

    bool Initialize(HWND hWnd)
    {
        this->SubclassWindow(hWnd);

        return true;
    }

    /// <summary>
    /// Handles EM_SETSEL to prevent the content from being selected.
    /// </summary>
    LRESULT OnSetSel(UINT, WPARAM, LPARAM, BOOL & handled)
    {
        return 0;
    }

private:
    int _Value;
};

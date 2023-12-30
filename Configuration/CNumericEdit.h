
/** $VER: CNumericEdit.h (2023.11.30) P. Stuer - Implements a numeric edit box using WTL. **/

#pragma once

#include "framework.h"

class CNumericEdit: public CWindowImpl<CNumericEdit, CEdit>
{
public:
    BEGIN_MSG_MAP(CNumericEdit)
        MESSAGE_HANDLER(EM_SETSEL, OnSetSel)
    END_MSG_MAP()

    /// <summary>
    /// Initializes the control.
    /// </summary>
    bool Initialize(HWND hWnd)
    {
        ATLASSERT(::IsWindow(hWnd));

        this->SubclassWindow(hWnd);

        return true;
    }

    /// <summary>
    /// Terminates the control.
    /// </summary>
    void Terminate()
    {
        if (!IsWindow())
            return;

        this->UnsubclassWindow(TRUE);
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

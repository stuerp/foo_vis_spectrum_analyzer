
/** $VER: CNumericEdit.h (2023.11.30) P. Stuer - Implements a numeric edit box using WTL. **/

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

class CNumericEdit: public CWindowImpl<CNumericEdit, CEdit>
{
public:
    CNumericEdit() : _IsSubclassed(false)
    {
    }

    /// <summary>
    /// Initializes the control.
    /// </summary>
    bool Initialize(HWND hWnd) noexcept
    {
        if (_IsSubclassed)
            return false;

        ATLASSERT(::IsWindow(hWnd));

        _IsSubclassed = SubclassWindow(hWnd);

        return true;
    }

    /// <summary>
    /// Terminates the control.
    /// </summary>
    void Terminate() noexcept
    {
        if (!IsWindow() || !_IsSubclassed)
            return;

        UnsubclassWindow(TRUE);
        _IsSubclassed = false;
    }

    /// <summary>
    /// Handles EM_SETSEL to prevent the content from being selected.
    /// </summary>
    LRESULT OnSetSel(UINT, WPARAM, LPARAM, BOOL & handled) noexcept
    {
        return 0;
    }

    BEGIN_MSG_MAP(CNumericEdit)
        MESSAGE_HANDLER(EM_SETSEL, OnSetSel)
    END_MSG_MAP()

private:
    bool _IsSubclassed;
    int _Value;
};

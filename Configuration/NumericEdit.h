
/** $VER: numeric_edit_t.h (2023.11.30) P. Stuer - Implements a numeric edit box using WTL. **/

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

#undef SubclassWindow

class numeric_edit_t: public CWindowImpl<numeric_edit_t, CEdit>
{
public:
    numeric_edit_t() = default;

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

    BEGIN_MSG_MAP(numeric_edit_t)
        MESSAGE_HANDLER(EM_SETSEL, OnSetSel)
    END_MSG_MAP()

private:
    bool _IsSubclassed { false };
    int _Value { 0 };
};

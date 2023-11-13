
/** $VER: ConfigurationDialog.h (2023.11.13) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

// ATL
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <atltypes.h>
#include <atlwin.h>

// WTL
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atlddx.h>
#include <atldlgs.h>
//#include <atldwm.h>
#include <atlfind.h>
#include <atlframe.h>
#include <atlgdi.h>
#include <atlmisc.h>
#include <atlprint.h>
#include <atlres.h>
#include <atlribbon.h>
#include <atlscrl.h>
#include <atlsplit.h>
#include <atltheme.h>
#include <atluser.h>
#include <atlwinx.h>

#include "Resources.h"
#include "Configuration.h"

/// <summary>
/// Implements the modeless Options dialog.
/// </summary>
class ConfigurationDialog : public CDialogImpl<ConfigurationDialog>, public CDialogResize<ConfigurationDialog>
{
public:
    ConfigurationDialog() : m_bMsgHandled(false), _Configuration() { }

    BEGIN_MSG_MAP_EX(ConfigurationDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
//      MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
        MSG_WM_CLOSE(OnClose)

        COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
        COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)

        CHAIN_MSG_MAP(CDialogResize<ConfigurationDialog>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(ConfigurationDialog)
        DLGRESIZE_CONTROL(IDC_FILENAME, DLSZ_SIZE_X)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    enum { IDD = IDD_CONFIGURATION };

private:
    /// <summary>
    /// Initializes the dialog.
    /// </summary>
    BOOL OnInitDialog(CWindow, LPARAM lParam)
    {
        _Configuration = (Configuration *) lParam;

        DlgResize_Init();

        if (_Configuration->_OptionsRect.right != -1)
            MoveWindow(&_Configuration->_OptionsRect);

        return TRUE;
    }

    /// <summary>
    /// Handles the Close message;
    /// </summary>
    void OnClose()
    {
        GetWindowRect(&_Configuration->_OptionsRect);
        SetMsgHandled(FALSE);
    }

    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND)
    {
        return (HBRUSH)::GetStockObject(DKGRAY_BRUSH);
    }

    void OnButton(UINT, int id, CWindow)
    {
        GetWindowRect(&_Configuration->_OptionsRect);

        DestroyWindow();
    }

private:
    Configuration * _Configuration;
};

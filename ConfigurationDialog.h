
/** $VER: ConfigurationDialog.h (2023.11.19) P. Stuer - Implements the configuration dialog. **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

// ATL
#include <atlbase.h>
#include <atlstr.h>

// WTL
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atldlgs.h>
#include <atlfind.h>
#include <atlframe.h>
#include <atlmisc.h>
#include <atlres.h>
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
    ConfigurationDialog() : m_bMsgHandled(false), _hParent() { }

    BEGIN_MSG_MAP_EX(ConfigurationDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
//      MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
        MSG_WM_CLOSE(OnClose)

    #pragma region FFT
        COMMAND_HANDLER_EX(IDC_TRANSFORM, CBN_SELCHANGE, OnSelectionChanged)
    #pragma endregion

    #pragma region FFT
        COMMAND_HANDLER_EX(IDC_FFT_SIZE, CBN_SELCHANGE, OnSelectionChanged)
        COMMAND_HANDLER_EX(IDC_FFT_SIZE_PARAMETER, EN_CHANGE, OnEditChange)

        COMMAND_HANDLER_EX(IDC_SCALING_FUNCTION, CBN_SELCHANGE, OnSelectionChanged)
        COMMAND_HANDLER_EX(IDC_SUMMATION_METHOD, CBN_SELCHANGE, OnSelectionChanged)
        COMMAND_HANDLER_EX(IDC_MAPPING_METHOD, CBN_SELCHANGE, OnSelectionChanged)

        COMMAND_HANDLER_EX(IDC_SMOOTHING_METHOD, CBN_SELCHANGE, OnSelectionChanged)
        COMMAND_HANDLER_EX(IDC_SMOOTHING_FACTOR, EN_CHANGE, OnEditChange)

        COMMAND_HANDLER_EX(IDC_SMOOTH_LOWER_FREQUENCIES, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_SMOOTH_GAIN_TRANSITION, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_KERNEL_SIZE, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_GAMMA, EN_CHANGE, OnEditChange)
    #pragma endregion

    #pragma region Frequencies
        COMMAND_HANDLER_EX(IDC_DISTRIBUTION, CBN_SELCHANGE, OnSelectionChanged)

        COMMAND_HANDLER_EX(IDC_NUM_BANDS, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_MIN_FREQUENCY, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_MAX_FREQUENCY, EN_CHANGE, OnEditChange)

        COMMAND_HANDLER_EX(IDC_MIN_NOTE, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_MAX_NOTE, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BANDS_PER_OCTAVE, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_PITCH, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_TRANSPOSE, EN_CHANGE, OnEditChange)

        COMMAND_HANDLER_EX(IDC_SKEW_FACTOR, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BANDWIDTH, EN_CHANGE, OnEditChange)
    #pragma endregion

    #pragma region X axis
        COMMAND_HANDLER_EX(IDC_X_AXIS, CBN_SELCHANGE, OnSelectionChanged)
    #pragma endregion

    #pragma region Y axis
        COMMAND_HANDLER_EX(IDC_Y_AXIS, CBN_SELCHANGE, OnSelectionChanged)

        COMMAND_HANDLER_EX(IDC_MIN_DECIBEL, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_MAX_DECIBEL, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_USE_ABSOLUTE, BN_CLICKED, OnButtonClick)
    #pragma endregion

    #pragma region Rendering
        COMMAND_HANDLER_EX(IDC_COLOR_SCHEME, CBN_SELCHANGE, OnSelectionChanged)
        COMMAND_HANDLER_EX(IDC_DRAW_BAND_BACKGROUND, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDC_PEAK_MODE, CBN_SELCHANGE, OnSelectionChanged)
        COMMAND_HANDLER_EX(IDC_HOLD_TIME, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_ACCELERATION, EN_CHANGE, OnEditChange)
    #pragma endregion

        COMMAND_HANDLER_EX(IDC_RESET, BN_CLICKED, OnButtonClick)

        COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
        COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)

        CHAIN_MSG_MAP(CDialogResize<ConfigurationDialog>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(ConfigurationDialog)
        DLGRESIZE_CONTROL(IDC_RESET, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    enum { IDD = IDD_CONFIGURATION };

private:
    #pragma region CDialogImpl
    /// <summary>
    /// Initializes the dialog.
    /// </summary>
    BOOL OnInitDialog(CWindow w, LPARAM lParam)
    {
        _hParent = (HWND) lParam;
        _OldConfiguration = _Configuration;

        DlgResize_Init();

        Initialize();

        if (_Configuration._DialogBounds.right != -1)
            MoveWindow(&_Configuration._DialogBounds);

        return TRUE;
    }

    /// <summary>
    /// Handles the Close message.
    /// </summary>
    void OnClose()
    {
        GetWindowRect(&_Configuration._DialogBounds);
        SetMsgHandled(FALSE);
    }

    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND)
    {
        return (HBRUSH)::GetStockObject(DKGRAY_BRUSH);
    }

    /// <summary>
    /// Handles the OK or Cancel button.
    /// </summary>
    void OnButton(UINT, int id, CWindow)
    {
        if (id == IDOK)
        {
            ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGED, 0, 0);
        }
        else
        if (id == IDCANCEL)
        {
            _Configuration = _OldConfiguration;

            ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
        }

        GetWindowRect(&_Configuration._DialogBounds);

        DestroyWindow();
    }

    void OnSelectionChanged(UINT, int, CWindow);
    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnButtonClick(UINT, int, CWindow);
    #pragma endregion

    void Initialize();
    void UpdateControls();

private:
    HWND _hParent;
    Configuration _OldConfiguration;
};

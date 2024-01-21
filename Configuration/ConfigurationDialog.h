
/** $VER: ConfigurationDialog.h (2024.01.21) P. Stuer - Implements the configuration dialog. **/

#pragma once

#include "framework.h"
#include "Support.h"

#include "Resources.h"
#include "Configuration.h"

#include "CMenuListBox.h"
#include "CNumericEdit.h"
#include "CColorButton.h"
#include "CColorListBox.h"
#include "CButtonMenu.h"

#include <sdk/coreDarkMode.h>

struct DialogParameters
{
    HWND _hWnd;
    Configuration * _Configuration;
};

/// <summary>
/// Implements the modeless Options dialog.
/// </summary>
class ConfigurationDialog : public CDialogImpl<ConfigurationDialog>, public CDialogResize<ConfigurationDialog>
{
public:
    ConfigurationDialog() : m_bMsgHandled(false), _hParent() { }

    ConfigurationDialog(const ConfigurationDialog &) = delete;
    ConfigurationDialog & operator=(const ConfigurationDialog &) = delete;
    ConfigurationDialog(ConfigurationDialog &&) = delete;
    ConfigurationDialog & operator=(ConfigurationDialog &&) = delete;

    virtual ~ConfigurationDialog() { }

    enum { IDD = IDD_CONFIGURATION };

private:
    #pragma region CDialogImpl
    BOOL OnInitDialog(CWindow w, LPARAM lParam);

    /// <summary>
    /// Handles the WM_CLOSE message.
    /// </summary>
    void OnClose()
    {
        GetWindowRect(&_Configuration->_DialogBounds);

        Terminate();

        SetMsgHandled(FALSE);
    }

    /// <summary>
    /// Handles the WM_COLORS_CHANGED message.
    /// </summary>
    LRESULT OnColorsChanged(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        UpdateColorControls();

        SetMsgHandled(TRUE);

        return 0;
    }

#ifdef _DEBUG
    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background. For layout debugging purposes.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND) const noexcept
    {
        return (HBRUSH)::GetStockObject(DKGRAY_BRUSH);
    }
#endif

    void Initialize();
    void Terminate();

    void OnSelectionChanged(UINT, int, CWindow);
    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept;
    void OnButtonClick(UINT, int, CWindow);

    LRESULT OnDeltaPos(LPNMHDR nmhd);
    LRESULT OnChanged(LPNMHDR nmhd);

    void OnChannels(UINT, int, HWND);

    void UpdateControls();
    void UpdateColorControls();
    void UpdateChannelsMenu();
    void UpdatePage1(int mode) const noexcept;
    void UpdatePage2(int mode) const noexcept;
    void UpdatePage3(int mode) const noexcept;

    static int ClampNewSpinPosition(LPNMUPDOWN nmud, int minValue, int maxValue) noexcept;
    static double ClampNewSpinPosition(LPNMUPDOWN nmud, double minValue, double maxValue, double scale) noexcept;

    void SetFrequency(int id, double frequency) noexcept;
    void SetNote(int id, uint32_t noteNumber) noexcept;
    void SetDecibel(int id, double decibel) noexcept;

    BEGIN_MSG_MAP_EX(ConfigurationDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
//      MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
        MSG_WM_CLOSE(OnClose)

        MESSAGE_HANDLER_EX(WM_COLORS_CHANGED, OnColorsChanged)

        COMMAND_RANGE_HANDLER_EX(IDM_CHANNELS_FIRST, IDM_CHANNELS_LAST, OnChannels);

        COMMAND_CODE_HANDLER_EX(CBN_SELCHANGE, OnSelectionChanged) // This also handles LBN_SELCHANGE
        COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEditChange)
        COMMAND_CODE_HANDLER_EX(EN_KILLFOCUS, OnEditLostFocus)
        COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnButtonClick)

        NOTIFY_CODE_HANDLER_EX(UDN_DELTAPOS, OnDeltaPos)
        NOTIFY_CODE_HANDLER_EX(NM_CHANGED, OnChanged)

        REFLECT_NOTIFICATIONS() // Required for CColorListBox

        CHAIN_MSG_MAP(CDialogResize<ConfigurationDialog>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(ConfigurationDialog)
        DLGRESIZE_CONTROL(IDC_MENULIST, DLSZ_SIZE_Y)
/*
        DLGRESIZE_CONTROL(IDC_BANDS, DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDC_GRADIENT, DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDC_COLOR_LIST, DLSZ_SIZE_Y)

            DLGRESIZE_CONTROL(IDC_SMOOTHING_METHOD, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_SMOOTHING_METHOD_LBL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_SMOOTHING_FACTOR, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_SMOOTHING_FACTOR_LBL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_PEAK_MODE, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_PEAK_MODE_LBL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_HOLD_TIME, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_HOLD_TIME_LBL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_ACCELERATION, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_ACCELERATION_LBL, DLSZ_MOVE_Y)
*/
        DLGRESIZE_CONTROL(IDC_RESET, DLSZ_MOVE_X | DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    #pragma endregion

private:
    HWND _hParent;

    Configuration * _Configuration;
    Configuration _OldConfiguration;

    CMenuListBox _MenuList;

    CNumericEdit _WindowParameter;
    CNumericEdit _WindowSkew;

    CButtonMenu _Channels;

    CNumericEdit _KernelSize;

    CNumericEdit _NumBands;
    CNumericEdit _LoFrequency;
    CNumericEdit _HiFrequency;
    CNumericEdit _MinNote;
    CNumericEdit _MaxNote;
    CNumericEdit _BandsPerOctave;
    CNumericEdit _Pitch;
    CNumericEdit _Transpose;
    CNumericEdit _SkewFactor;
    CNumericEdit _Bandwidth;

    CNumericEdit _AmplitudeLo;
    CNumericEdit _AmplitudeHi;
    CNumericEdit _AmplitudeStep;

    CNumericEdit _Gamma;

    CColorButton _Gradient;
    CColorListBox _Colors;
    CNumericEdit _Position;

    CNumericEdit _ArtworkOpacity;
    CNumericEdit _ArtworkColors;
    CNumericEdit _LightnessThreshold;

    CColorButton _BackColor;
    CColorButton _XTextColor;
    CColorButton _XLineColor;
    CColorButton _YTextColor;
    CColorButton _YLineColor;
    CColorButton _LiteBandColor;
    CColorButton _DarkBandColor;

    CNumericEdit _LineWidth;
    CNumericEdit _AreaOpacity;

    fb2k::CCoreDarkModeHooks _DarkMode;
};


/** $VER: ConfigurationDialog.h (2023.11.24) P. Stuer - Implements the configuration dialog. **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

// ATL
#include <atlbase.h>

// WTL
#include <atlapp.h>
#include <atlframe.h>

#include "Resources.h"
#include "Configuration.h"

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

    BEGIN_MSG_MAP_EX(ConfigurationDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_DPICHANGED(OnDPIChanged)
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
        NOTIFY_HANDLER_EX(IDC_MIN_FREQUENCY_SPIN, UDN_DELTAPOS, OnMinFrequencyDeltaPos)
        NOTIFY_HANDLER_EX(IDC_MAX_FREQUENCY_SPIN, UDN_DELTAPOS, OnMaxFrequencyDeltaPos)

        NOTIFY_HANDLER_EX(IDC_MIN_NOTE_SPIN, UDN_DELTAPOS, OnMinNoteDeltaPos)
        NOTIFY_HANDLER_EX(IDC_MAX_NOTE_SPIN, UDN_DELTAPOS, OnMaxNoteDeltaPos)

        COMMAND_HANDLER_EX(IDC_BANDS_PER_OCTAVE, EN_CHANGE, OnEditChange)
        NOTIFY_HANDLER_EX(IDC_PITCH_SPIN, UDN_DELTAPOS, OnPitchDeltaPos)
        COMMAND_HANDLER_EX(IDC_TRANSPOSE, EN_CHANGE, OnEditChange)

        COMMAND_HANDLER_EX(IDC_SKEW_FACTOR, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BANDWIDTH, EN_CHANGE, OnEditChange)
    #pragma endregion

    #pragma region X axis
        COMMAND_HANDLER_EX(IDC_X_AXIS, CBN_SELCHANGE, OnSelectionChanged)
    #pragma endregion

    #pragma region Y axis
        COMMAND_HANDLER_EX(IDC_Y_AXIS, CBN_SELCHANGE, OnSelectionChanged)

        NOTIFY_HANDLER_EX(IDC_MIN_DECIBEL_SPIN, UDN_DELTAPOS, OnMinDecibelDeltaPos)
        NOTIFY_HANDLER_EX(IDC_MAX_DECIBEL_SPIN, UDN_DELTAPOS, OnMaxDecibelDeltaPos)

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
        DialogParameters * dp = (DialogParameters *) lParam;

        _hParent = dp->_hWnd;
        _Configuration = dp->_Configuration;

        _OldConfiguration = *_Configuration;

        DlgResize_Init();

        Initialize();

        if (_Configuration->_DialogBounds.right != -1)
            MoveWindow(&_Configuration->_DialogBounds);

        _DPI = (FLOAT) ::GetDpiForWindow(m_hWnd);

        return TRUE;
    }

    /// <summary>
    /// Handles a DPI change.
    /// </summary>
    LRESULT OnDPIChanged(UINT dpiX, UINT dpiY, PRECT newRect)
    {
        return 0;
    }

    /// <summary>
    /// Handles the Close message.
    /// </summary>
    void OnClose()
    {
        GetWindowRect(&_Configuration->_DialogBounds);
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
            *_Configuration = _OldConfiguration;

            ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);
        }

        GetWindowRect(&_Configuration->_DialogBounds);

        DestroyWindow();
    }

    void OnSelectionChanged(UINT, int, CWindow);
    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnButtonClick(UINT, int, CWindow);

    /// <summary>
    /// Handles a notification from an UpDown control.
    /// </summary>
    LRESULT OnMinNoteDeltaPos(LPNMHDR nmhd)
    {
        LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

        _Configuration->_MinNote = (uint32_t) nmud->iPos;
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

        SetNote(IDC_MIN_NOTE, _Configuration->_MinNote);

        return 0;
    }

    /// <summary>
    /// Handles a notification from an UpDown control.
    /// </summary>
    LRESULT OnMaxNoteDeltaPos(LPNMHDR nmhd)
    {
        LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

        _Configuration->_MaxNote = (uint32_t) nmud->iPos;
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

        SetNote(IDC_MAX_NOTE, _Configuration->_MaxNote);

        return 0;
    }

    /// <summary>
    /// Sets the display version of the note number.
    /// </summary>
    void SetNote(int id, uint32_t noteNumber)
    {
        static const WCHAR * Notes[] = { L"C%d", L"C#%d", L"D%d", L"D#%d", L"E%d", L"F%d", L"F#%d", L"G%d", L"G#%d", L"A%d", L"A#%d", L"B%d" };

        WCHAR Text[16] = { };

        size_t NoteIndex = (size_t) (noteNumber % 12);
        int Octave = (int) ((noteNumber + 3) / 12);

        ::StringCchPrintfW(Text, _countof(Text), Notes[NoteIndex], Octave);

        SetDlgItemTextW(id, Text);
    }

    /// <summary>
    /// Handles a notification from an UpDown control.
    /// </summary>
    LRESULT OnMinFrequencyDeltaPos(LPNMHDR nmhd)
    {
        LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

        _Configuration->_MinFrequency = (uint32_t) (nmud->iPos / 100);
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

        SetFrequency(IDC_MIN_FREQUENCY, _Configuration->_MinFrequency);

        return 0;
    }

    /// <summary>
    /// Handles a notification from an UpDown control.
    /// </summary>
    LRESULT OnMaxFrequencyDeltaPos(LPNMHDR nmhd)
    {
        LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

        _Configuration->_MaxFrequency = (uint32_t) (nmud->iPos / 100);
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

        SetFrequency(IDC_MAX_FREQUENCY, _Configuration->_MaxFrequency);

        return 0;
    }

    /// <summary>
    /// Handles a notification from an UpDown control.
    /// </summary>
    LRESULT OnPitchDeltaPos(LPNMHDR nmhd)
    {
        LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

        _Configuration->_Pitch = (double) nmud->iPos / 100.;
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

        SetFrequency(IDC_PITCH, _Configuration->_Pitch);

        return 0;
    }

    /// <summary>
    /// Sets the display version of the frequency.
    /// </summary>
    void SetFrequency(int id, double frequency)
    {
        WCHAR Text[16] = { };

        ::StringCchPrintfW(Text, _countof(Text), L"%.f", frequency);

        SetDlgItemTextW(id, Text);
    }

    /// <summary>
    /// Handles a notification from an UpDown control.
    /// </summary>
    LRESULT OnMinDecibelDeltaPos(LPNMHDR nmhd)
    {
        LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

        _Configuration->_MinDecibel = (double) nmud->iPos / 10.f;
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

        SetDecibel(IDC_MIN_DECIBEL, _Configuration->_MinDecibel);

        return 0;
    }

    /// <summary>
    /// Handles a notification from an UpDown control.
    /// </summary>
    LRESULT OnMaxDecibelDeltaPos(LPNMHDR nmhd)
    {
        LPNMUPDOWN nmud = (LPNMUPDOWN) nmhd;

        _Configuration->_MaxDecibel = (double) nmud->iPos / 10.f;
        ::SendMessageW(_hParent, WM_CONFIGURATION_CHANGING, 0, 0);

        SetDecibel(IDC_MAX_DECIBEL, _Configuration->_MaxDecibel);

        return 0;
    }

    /// <summary>
    /// Sets the display version of the amplitude.
    /// </summary>
    void SetDecibel(int id, double frequency)
    {
        WCHAR Text[16] = { };

        ::StringCchPrintfW(Text, _countof(Text), L"%.1f", frequency);

        SetDlgItemTextW(id, Text);
    }
    #pragma endregion

    void Initialize();
    void UpdateControls();

private:
    HWND _hParent;
    Configuration * _Configuration;
    Configuration _OldConfiguration;
    float _DPI;
};


/** $VER: ConfigurationDialog.h (2023.11.25) P. Stuer - Implements the configuration dialog. **/

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

        COMMAND_CODE_HANDLER_EX(CBN_SELCHANGE, OnSelectionChanged)
        COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEditChange)
        NOTIFY_CODE_HANDLER_EX(UDN_DELTAPOS, OnDeltaPos)
        COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnButtonClick)

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

    void Initialize();

    void OnSelectionChanged(UINT, int, CWindow);
    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnButtonClick(UINT, int, CWindow);
    LRESULT OnDeltaPos(LPNMHDR nmhd);

    void UpdateControls();

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
    /// Sets the display version of the amplitude.
    /// </summary>
    void SetDecibel(int id, double decibel)
    {
        WCHAR Text[16] = { };

        ::StringCchPrintfW(Text, _countof(Text), L"%.1f", decibel);

        SetDlgItemTextW(id, Text);
    }
    #pragma endregion

private:
    HWND _hParent;
    Configuration * _Configuration;
    Configuration _OldConfiguration;
    float _DPI;
};

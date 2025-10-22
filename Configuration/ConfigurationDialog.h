
/** $VER: ConfigurationDialog.h (2025.10.15) P. Stuer - Implements the configuration dialog. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Resources.h"
#include "State.h"

#include "CMenuListBox.h"
#include "CNumericEdit.h"
#include "CColorButton.h"
#include "CColorListBox.h"

#include "StyleManager.h"

struct DialogParameters
{
    HWND _hWnd;
    state_t * _State;
};

/// <summary>
/// Implements the modeless Options dialog.
/// </summary>
class ConfigurationDialog : public CDialogImpl<ConfigurationDialog>, public CDialogResize<ConfigurationDialog>
{
public:
    ConfigurationDialog() : m_bMsgHandled(false), _hParent(), _IsInitializing() { }

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
        GetWindowRect(&_State->_DialogRect);

        Terminate();

        SetMsgHandled(FALSE);
    }

    LRESULT OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam);

    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background. For layout debugging purposes.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND) const noexcept
    {
    #ifdef _DEBUG
        return ::CreateSolidBrush(RGB(250, 250, 250));
    #else
        return FALSE;
    #endif
    }

    void Initialize();
    void Terminate();

    void ConfigurationChanged(Settings settings) const noexcept;

    void OnSelectionChanged(UINT, int, CWindow);
    void OnDoubleClick(UINT, int, CWindow);
    void OnEditChange(UINT, int, CWindow) noexcept;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept;
    void OnButtonClick(UINT, int, CWindow);

    LRESULT OnDeltaPos(LPNMHDR nmhd);
    LRESULT OnChanged(LPNMHDR nmhd);

    void UpdatePages(size_t index) const noexcept;

    void UpdateVisualizationPage() noexcept;
    void UpdateTransformPage() noexcept;
    void UpdateFrequenciesPage() const noexcept;
    void UpdateFiltersPage() const noexcept;
    void UpdateCommonPage() const noexcept;
    void UpdateGraphsPage() noexcept;
    void UpdateStylesPage() noexcept;
    void UpdatePresetsPage() const noexcept;

    void UpdateColorControls();
    void UpdateCurrentColor(style_t * style) const noexcept;
    void UpdateGradientStopPositons(style_t * style, size_t index) const noexcept;
    void GetPresetNames() noexcept;
    void GetPreset(const std::wstring & presetName) noexcept;

    static int ClampNewSpinPosition(LPNMUPDOWN nmud, int minValue, int maxValue) noexcept;
    static double ClampNewSpinPosition(LPNMUPDOWN nmud, double minValue, double maxValue, double scale) noexcept;

    void SetInteger(int id, int64_t value) noexcept;
    void SetDouble(int id, double value, unsigned width = 0, unsigned precision = 2) noexcept;
    void SetNote(int id, uint32_t noteNumber) noexcept;

    void InitializeXAxisMode() noexcept;
    void InitializeYAxisMode() noexcept;
    void InitializeStyles() noexcept;

    BEGIN_MSG_MAP_EX(ConfigurationDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CLOSE(OnClose)

        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
//      MSG_WM_MOUSEMOVE(OnMouseMove)

        MESSAGE_HANDLER_EX(UM_CONFIGURATION_CHANGED, OnConfigurationChanged)

        COMMAND_CODE_HANDLER_EX(CBN_SELCHANGE, OnSelectionChanged) // This also handles LBN_SELCHANGE
        COMMAND_CODE_HANDLER_EX(LBN_DBLCLK, OnDoubleClick)
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

        DLGRESIZE_CONTROL(IDC_RESET, DLSZ_MOVE_X | DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    #pragma endregion

private:
    HWND _hParent;

    std::vector<VisualElement> _ActiveStyles;   // The styles relevant to the current visualization.
    size_t _SelectedStyle;                      // Index of the selected style in the listbox.

    size_t _SelectedGraph;                      // Index of the selected graph in the listbox.

    CToolTipCtrl _ToolTipControl;

    state_t * _State;
    state_t _OldState;
    bool _IsInitializing;
    bool _IgnoreNotifications;

    CMenuListBox _MenuList;

    std::vector<CNumericEdit *> _NumericEdits;

    CColorButton _Color;
    CColorButton _Gradient;
    CColorListBox _Colors;

    std::vector<std::wstring> _PresetNames;

    fb2k::CCoreDarkModeHooks _DarkMode;
};

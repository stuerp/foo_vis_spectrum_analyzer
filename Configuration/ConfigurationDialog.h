
/** $VER: ConfigurationDialog.h (2026.02.22) P. Stuer - Implements the configuration dialog. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "DialogParameters.h"
#include "Resources.h"
#include "State.h"

#include "CMenuListBox.h"
#include "CNumericEdit.h"
#include "CColorButton.h"
#include "CColorListBox.h"

#include "StyleManager.h"

#include "VisualizationPage.h"
#include "TransformPage.h"
#include "FrequenciesPage.h"
#include "FiltersPage.h"
#include "CommonPage.h"
#include "GraphsPage.h"
#include "StylesPage.h"
#include "PresetsPage.h"

/// <summary>
/// Implements the modeless configuration dialog.
/// </summary>
class configuration_dialog_t : public CDialogImpl<configuration_dialog_t>, public CDialogResize<configuration_dialog_t>
{
public:
    configuration_dialog_t() : m_bMsgHandled(false), _hParent(), _IsInitializing() { }

    configuration_dialog_t(const configuration_dialog_t &) = delete;
    configuration_dialog_t & operator=(const configuration_dialog_t &) = delete;
    configuration_dialog_t(configuration_dialog_t &&) = delete;
    configuration_dialog_t & operator=(configuration_dialog_t &&) = delete;

    virtual ~configuration_dialog_t() noexcept { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept;

    LRESULT OnSize(UINT msg, WPARAM wParam, LPARAM lParam, BOOL & isHandled) noexcept;
    HBRUSH OnCtlColorDlg(HDC, HWND) const noexcept;

    void OnSelectionChanged(UINT, int, CWindow) noexcept;
    void OnButtonClick(UINT, int, CWindow) noexcept;

    LRESULT OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

    BEGIN_MSG_MAP_EX(configuration_dialog_t)
        MSG_WM_INITDIALOG(OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)

        COMMAND_CODE_HANDLER_EX(CBN_SELCHANGE, OnSelectionChanged) // This also handles LBN_SELCHANGE
        COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnButtonClick)

        MESSAGE_HANDLER_EX(UM_CONFIGURATION_CHANGED, OnConfigurationChanged)

        REFLECT_NOTIFICATIONS() // Required for CMenuListBBox and CColorListBox

        CHAIN_MSG_MAP(CDialogResize<configuration_dialog_t>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(configuration_dialog_t)
        DLGRESIZE_CONTROL(IDC_MENULIST, DLSZ_SIZE_Y)

        DLGRESIZE_CONTROL(IDC_RESET, DLSZ_MOVE_X | DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

private:
    void InitializeControls() noexcept;
    void TerminateControls() noexcept;

    void ConfigurationChanged(Settings settings) const noexcept;

    void UpdatePages(size_t index) const noexcept;
    void ResizePages() noexcept;

public:
    enum { IDD = IDD_MAIN_DIALOG };

private:
    const dialog_parameters_t * _DialogParameters;

    HWND _hParent;

    state_t * _State;
    state_t _OldState;
    bool _IsInitializing;
    bool _IgnoreNotifications;

    CMenuListBox _MenuList;
    std::vector<std::shared_ptr<page_t>> _Pages;
    CToolTipCtrl _ToolTipControl;

    fb2k::CCoreDarkModeHooks _DarkMode;
};

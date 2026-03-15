
/** $VER: ConfigurationDialog.cpp (2026.03.13) P. Stuer - Implements the configuration dialog. **/

#include "pch.h"

#include "ConfigurationDialog.h"
#include "Gradients.h"
#include "Layout.h"
#include "CColorDialogEx.h"
#include "PresetManager.h"
#include "Support.h"

#include "Direct2D.h"
#include "Theme.h"

#include "Log.h"

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL configuration_dialog_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    _IsInitializing = true;

    _Theme.Initialize(_DarkMode);

    DlgResize_Init(true, true, WS_CLIPCHILDREN);

    _DialogParameters = (const dialog_parameters_t *) lParam;

    _hParent = _DialogParameters->_hWnd;
    _State   = _DialogParameters->_State;

    if (IsRectEmpty(&_State->_DialogRect))
    {
        _State->_DialogRect.right  = W_A00;
        _State->_DialogRect.bottom = H_A00;

        ::MapDialogRect(m_hWnd, &_State->_DialogRect);
    }

    _OldState = *_State;

    MoveWindow(&_State->_DialogRect);

    _DarkMode.AddDialogWithControls(*this);

    {
        InitializeControls();

        ResizePages();

        if (_State->_PageIndex < _Pages.size())
            _Pages[_State->_PageIndex]->ShowWindow(SW_SHOW);
    }

    _IsInitializing = false;

    return TRUE;
}

/// <summary>
/// Handles a WM_SIZE message.
/// </summary>
LRESULT configuration_dialog_t::OnSize(UINT msg, WPARAM wParam, LPARAM lParam, BOOL & handled) noexcept
{
//    CDialogResize<NewConfigurationDialog>::OnSize(msg, wParam, lParam, handled);
    __super::OnSize(msg, wParam, lParam, handled);

    if (wParam != SIZE_MINIMIZED)
        ResizePages();

    return 0;
}

/// <summary>
/// Returns a brush that the system uses to draw the dialog background. For layout debugging purposes.
/// </summary>
HBRUSH configuration_dialog_t::OnCtlColorDlg(HDC, HWND) const noexcept
{
#ifdef _DEBUG
    return ::CreateSolidBrush(RGB(240, 240, 240));
#else
    return FALSE;
#endif
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void configuration_dialog_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    if ((_State == nullptr) || (id != IDC_MENULIST))
        return;

    const size_t Selection = (size_t) _MenuList.GetCurSel();

    _State->_PageIndex = Selection;

    size_t i = 0;

    for (auto & Page : _Pages)
        Page->ShowWindow((i++ == Selection) ? SW_SHOW : SW_HIDE);
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void configuration_dialog_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    switch (id)
    {
        default:
            return;

        case IDC_RESET:
        {
            _State->Reset();
            CfgLogLevel = (int64_t) DefaultCfgLogLevel;

            {
                for (auto & Page : _Pages)
                    Page->ShowWindow(SW_HIDE);

                _Pages[_State->_PageIndex]->ShowWindow(SW_SHOW); // Re-showing the pages also initializes the controls of that page.

                _MenuList.SetCurSel((int) _State->_PageIndex);
            }

            ConfigurationChanged(ConfigurationChanges::All);
            break;
        }

        case IDOK:
        case IDCANCEL:
        {
            if (id == IDCANCEL)
            {
                *_State = _OldState;

                ConfigurationChanged(ConfigurationChanges::All);
            }

            GetWindowRect(&_State->_DialogRect);

            TerminateControls(); // Don't call from WM_DESTROY handler.
            DestroyWindow();

            _State = nullptr;
            break;
        }
    }
}

/// <summary>
/// Handles the UM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT configuration_dialog_t::OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    for (auto & Page : _Pages)
        Page->OnConfigurationChanged(msg, wParam, lParam);

    SetMsgHandled(TRUE);

    return 0;
}

/// <summary>
/// Initializes the controls of the dialog.
/// </summary>
void configuration_dialog_t::InitializeControls() noexcept
{
    // Initialize the menu items.
    {
        _MenuList.Initialize(GetDlgItem(IDC_MENULIST));

        _MenuList.ResetContent();

        for (const auto & x : { L"Visualization", L"Transform", L"Frequencies", L"Filters", L"Common", L"Graphs", L"Styles", L"Presets" })
            _MenuList.AddString(x);

        _MenuList.SetCurSel((int) _State->_PageIndex);
    }

    // Create the pages.
    {
        std::shared_ptr<page_t> Page = std::make_shared<visualization_page_t>(IDD_VISUALIZATION_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);

        Page = std::make_shared<transform_page_t>(IDD_TRANSFORM_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);

        Page = std::make_shared<frequencies_page_t>(IDD_FREQUENCIES_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);

        Page = std::make_shared<filters_page_t>(IDD_FILTERS_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);

        Page = std::make_shared<common_page_t>(IDD_COMMON_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);

        Page = std::make_shared<graphs_page_t>(IDD_GRAPHS_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);

        Page = std::make_shared<styles_page_t>(IDD_STYLES_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);

        Page = std::make_shared<presets_page_t>(IDD_PRESETS_PAGE);

        Page->Create(m_hWnd, (LPARAM) _DialogParameters);
        _Pages.push_back(Page);
    }

    // Create the tooltip control.
    {
        _ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE);

        const std::unordered_map<int, const char *> Tips =
        {
            { IDC_RESET, "Resets the configuration to the default values." },
            { IDOK, "Closes the dialog box and makes the changes to the configuration final." },
            { IDCANCEL, "Closes the dialog box and undoes any changes to the configuration." },
        };

        for (const auto & [ID, Text] : Tips)
            _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

        _ToolTipControl.SetMaxTipWidth(200);
        ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);
    }
}

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void configuration_dialog_t::TerminateControls() noexcept
{
    // Delete the tooltip control.
    {
        if (_ToolTipControl.IsWindow())
            _ToolTipControl.DestroyWindow();
    }

    // Delete the pages.
    {
        for (auto & Page : _Pages)
            Page->DestroyWindow();

        _Pages.clear();
    }

    // Delete the menu list.
    _MenuList.Terminate();
}

/// <summary>
/// Resizes the child dialog pages.
/// </summary>
void configuration_dialog_t::ResizePages() noexcept
{
    auto Menu = GetDlgItem(IDC_MENULIST);
    RECT MenuRect = { };
    RECT MenuClientRect = { };

    if (Menu.IsWindow())
    {
        Menu.GetWindowRect(&MenuRect);

        MenuClientRect = MenuRect;

        ScreenToClient(&MenuClientRect);
    }

    auto Button = GetDlgItem(IDCANCEL);
    RECT ButtonRect = { };

    if (Button.IsWindow())
        Button.GetWindowRect(&ButtonRect);

    RECT wr =
    {
        .left   = MenuRect.right + MenuClientRect.left,
        .top    = MenuRect.top,
        .right  = ButtonRect.right,
        .bottom = ButtonRect.top - MenuClientRect.top
    };

    ScreenToClient(&wr);

    for (auto & Page : _Pages)
        Page->MoveWindow(&wr);
}

/// <summary>
/// Notifies the UI thread of the changed settings.
/// </summary>
void configuration_dialog_t::ConfigurationChanged(ConfigurationChanges settings) const noexcept
{
    if (_IsInitializing)
        return;

    ::PostMessageW(_hParent, UM_CONFIGURATION_CHANGED, (WPARAM) settings, 0);

    Log.AtDebug().Write(STR_COMPONENT_BASENAME " configuration dialog notified parent of a configuration change (%08X).", (uint32_t) settings);
}

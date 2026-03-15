
/** $VER: PresetsPage.cpp (2026.03.01) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "PresetsPage.h"
#include "PresetManager.h"

#include "Support.h"
#include "Log.h"

/// <summary>
/// Initializes the page.
/// </summary>
BOOL presets_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    DlgResize_Init(false, true); // This page has resizable controls.

    const std::unordered_map<int, const char *> Tips =
    {
        { IDC_PRESETS_ROOT, "Specifies the location of the preset files." },
        { IDC_PRESETS_ROOT_SELECT, "Opens a dialog to select a location." },
        { IDC_PRESET_NAMES, "Lists the presets in the current preset location." },
        { IDC_PRESET_NAME, "Specifies the name of the preset." },
        { IDC_PRESET_LOAD, "Loads and activates the specified preset." },
        { IDC_PRESET_SAVE, "Saves the current configuration as a preset." },
        { IDC_PRESET_DELETE, "Deletes the specified preset." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void presets_page_t::InitializeControls() noexcept
{
    {
        SetDlgItemTextW(IDC_PRESETS_ROOT, _State->_PresetsDirectoryPath.c_str());

        FillPresetListBox();
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of the page.
/// </summary>
void presets_page_t::UpdateControls() noexcept
{
    WCHAR PresetName[MAX_PATH] = { };

    GetDlgItemTextW(IDC_PRESET_NAME, PresetName, _countof(PresetName));

    const bool HasName = PresetName[0] != '\0';

    GetDlgItem(IDC_PRESET_LOAD).  EnableWindow(HasName);
    GetDlgItem(IDC_PRESET_SAVE).  EnableWindow(HasName);
    GetDlgItem(IDC_PRESET_DELETE).EnableWindow(HasName);
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void presets_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    if ((_State == nullptr) || (id != IDC_PRESET_NAMES))
        return;

    auto lb = (CListBox) GetDlgItem(IDC_PRESET_NAMES);

    const int SelectedIndex = lb.GetCurSel();

    if (!msc::InRange(SelectedIndex, 0, (int) _PresetNames.size() - 1))
        return;

    SetDlgItemTextW(IDC_PRESET_NAME, _PresetNames[(size_t) SelectedIndex].c_str());

    UpdateControls();
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void presets_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications || (code != EN_CHANGE))
        return;

    WCHAR Text[MAX_PATH] = { };

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
        default:
            return;

        case IDC_PRESETS_ROOT:
        {
            _State->_PresetsDirectoryPath = Text;

            FillPresetListBox();

            return;
        }

        case IDC_PRESET_NAME:
        {
            UpdateControls();

            return;
        }
    }
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void presets_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    if (id == IDC_PRESETS_ROOT)
        FillPresetListBox();
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void presets_page_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    switch (id)
    {
        default:
            return;

        case IDC_PRESETS_ROOT_SELECT:
        {
            pfc::string DirectoryPath = pfc::utf8FromWide(_State->_PresetsDirectoryPath.c_str());

            if (::uBrowseForFolder(m_hWnd, "Locate preset files...", DirectoryPath))
            {
                _State->_PresetsDirectoryPath = pfc::wideFromUTF8(DirectoryPath);

                pfc::wstringLite w = pfc::wideFromUTF8(DirectoryPath);

                SetDlgItemTextW(IDC_PRESETS_ROOT, pfc::wideFromUTF8(DirectoryPath));

                FillPresetListBox();
            }
            break;
        }

        case IDC_PRESET_LOAD:
        {
            WCHAR PresetName[MAX_PATH] = { };

            GetDlgItemTextW(IDC_PRESET_NAME, PresetName, _countof(PresetName));

            GetPreset(PresetName);
            FillPresetListBox();

            _IsInitializing = true;

            UpdateControls();

            _IsInitializing = false;

            ConfigurationChanged(ConfigurationChanges::All);
            break;
        }

        case IDC_PRESET_SAVE:
        {
            WCHAR PresetName[MAX_PATH] = { };

            GetDlgItemTextW(IDC_PRESET_NAME, PresetName, _countof(PresetName));

            PresetManager::Save(_State->_PresetsDirectoryPath, PresetName, _State);

            FillPresetListBox();
            break;
        }

        case IDC_PRESET_DELETE:
        {
            WCHAR PresetName[MAX_PATH] = { };

            GetDlgItemTextW(IDC_PRESET_NAME, PresetName, _countof(PresetName));

            PresetManager::Delete(_State->_PresetsDirectoryPath, PresetName);

            FillPresetListBox();
            break;
        }
    }
}

/// <summary>
/// Handles a double click on a list box item.
/// </summary>
void presets_page_t::OnDoubleClick(UINT code, int id, CWindow) noexcept
{
    if (_State == nullptr)
        return;

    if ((id == IDC_PRESET_NAMES) && (code == LBN_DBLCLK))
    {
        auto lb = (CListBox) GetDlgItem(id);

        const int SelectedIndex = lb.GetCurSel();

        if (!msc::InRange(SelectedIndex, 0, (int) _PresetNames.size() - 1))
            return;

        std::wstring PresetName = _PresetNames[(size_t) SelectedIndex];

        SetDlgItemTextW(IDC_PRESET_NAME, PresetName.c_str());

        GetPreset(PresetName);
        FillPresetListBox();

        UpdateControls();

        ConfigurationChanged(ConfigurationChanges::All);
    }
    else
        SetMsgHandled(FALSE);
}

/// <summary>
/// Updates the preset file list box.
/// </summary>
void presets_page_t::FillPresetListBox() noexcept
{
    _PresetNames.clear();

    auto w = (CListBox) GetDlgItem(IDC_PRESET_NAMES);

    w.ResetContent();

    int Count = 0;
    int SelectedIndex = -1;

    if (PresetManager::GetPresetNames(_State->_PresetsDirectoryPath, _PresetNames))
    {
        for (auto & PresetName : _PresetNames)
        {
            w.AddString(PresetName.c_str());

            if (PresetName == _State->_ActivePresetName)
                SelectedIndex = Count;

            Count++;
        }
    }

    w.SetCurSel(SelectedIndex);
}

/// <summary>
/// Loads and activates a preset.
/// </summary>
void presets_page_t::GetPreset(const std::wstring & presetName) noexcept
{
    state_t NewState;

    PresetManager::Load(_State->_PresetsDirectoryPath, presetName, &NewState);

    NewState._StyleManager.DominantColor       = _State->_StyleManager.DominantColor;
    NewState._StyleManager.UserInterfaceColors = _State->_StyleManager.UserInterfaceColors;

    NewState._StyleManager.UpdateCurrentColors();

    NewState._ActivePresetName = presetName;

    *_State = NewState;

    InitializeControls();
}

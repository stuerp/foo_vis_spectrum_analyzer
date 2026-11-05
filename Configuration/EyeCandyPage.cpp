
/** $VER: EyeCandyPage.cpp (2026.11.05) P. Stuer - Implements a configuration dialog page. **/

#include "pch.h"

#include "EyeCandyPage.h"

/// <summary>
/// Initializes the page.
/// </summary>
BOOL eye_candy_page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    __super::OnInitDialog(w, lParam);

    DlgResize_Init(false, true); // This page has resizable controls.

    static const std::unordered_map<int, const char *> Tips =
    {
        { IDC_EFFECT_CATEGORY,      "Specifies the eye candy effect category." },
        { IDC_EFFECT,               "Selects the eye candy effect." },

        { IDC_EFFECT_QUALITY,       "Configures the overall parameters that control the render quality of the effect." },
        { IDC_EFFECT_SPEED,         "Configures the overall parameters that control the render speed of the effect." },
        { IDC_EFFECT_BRIGHTNESS,    "Configures the overall parameters that control the brightness of the effect." },
        { IDC_EFFECT_DENSITY,       "Configures the overall parameters that control the density of the effect." },
    };

    for (const auto & [ID, Text] : Tips)
        _ToolTipControl.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR) GetDlgItem(ID).m_hWnd, nullptr, (LPWSTR) msc::UTF8ToWide(Text).c_str()));

    return TRUE;
}

/// <summary>
/// Initializes the controls of the page.
/// </summary>
void eye_candy_page_t::InitializeControls() noexcept
{
    // Effect Category
    {
        auto w = (CComboBox) GetDlgItem(IDC_EFFECT_CATEGORY);

        w.ResetContent();

        for (const auto & x : EffectCategories)
            w.AddString(x.Name.c_str());

        auto it = std::ranges::find_if(EffectCategories, [&](effect_category_t ec) { return ec.Id == _State->_EffectCategoryId; });

        if (it != EffectCategories.end())
            w.SetCurSel((int) std::ranges::distance(EffectCategories.begin(), it));
    }

    // Effect Quality
    {
        auto w = (CComboBox) GetDlgItem(IDC_EFFECT_QUALITY);

        w.ResetContent();

        for (const auto & x : { L"Low", L"Medium", L"High" })
            w.AddString(x);

        w.SetCurSel((int) _State->_EffectQuality);
    }

    // Effect Speed
    {
        auto w = (CComboBox) GetDlgItem(IDC_EFFECT_SPEED);

        w.ResetContent();

        for (const auto & x : { L"Slow", L"Normal", L"Fast" })
            w.AddString(x);

        w.SetCurSel((int) _State->_EffectSpeed);
    }

    // Effect Brightness
    {
        auto w = (CComboBox) GetDlgItem(IDC_EFFECT_BRIGHTNESS);

        w.ResetContent();

        for (const auto & x : { L"Low", L"Medium", L"High" })
            w.AddString(x);

        w.SetCurSel((int) _State->_EffectBrightness);
    }

    // Effect Density
    {
        auto w = (CComboBox) GetDlgItem(IDC_EFFECT_DENSITY);

        w.ResetContent();

        for (const auto & x : { L"Low", L"Medium", L"High" })
            w.AddString(x);

        w.SetCurSel((int) _State->_EffectDensity);
    }

    UpdateControls();
}

/// <summary>
/// Updates the controls of this page.
/// </summary>
void eye_candy_page_t::UpdateControls() noexcept
{
    UpdateEffect();

    const bool IsEyeCandy = (_State->_VisualizationType == VisualizationType::EyeCandy);

    for (const auto & Iter :
    {
        IDC_EFFECT_CATEGORY,
        IDC_EFFECT,
        IDC_EFFECT_QUALITY,
        IDC_EFFECT_SPEED,
        IDC_EFFECT_BRIGHTNESS,
        IDC_EFFECT_DENSITY,
    })
        GetDlgItem(Iter).EnableWindow(IsEyeCandy);
}

/// <summary>
/// Updates the Effects combobox.
/// </summary>
void eye_candy_page_t::UpdateEffect() noexcept
{
    const auto Category = EffectCategories[(size_t) _State->_EffectCategoryId];

    auto w = (CComboBox) GetDlgItem(IDC_EFFECT);

    w.ResetContent();

    for (const auto & x : Category.Effects)
        w.AddString(x.Name.c_str());

    auto it = std::ranges::find_if(Category.Effects, [&](effect_descriptor_t ed) { return ed.Id == _State->_EffectId; });

    if (it != Category.Effects.end())
        w.SetCurSel((int) std::ranges::distance(Category.Effects.begin(), it));
}

/// <summary>
/// Handles an update of the selected item of a combo box.
/// </summary>
void eye_candy_page_t::OnSelectionChanged(UINT notificationCode, int id, CWindow w) noexcept
{
    auto ChangedSettings = ConfigurationChanges::Renderer;

    auto cb = (CComboBox) w;

    const int SelectedIndex = cb.GetCurSel();

    switch (id)
    {
        case IDC_EFFECT_CATEGORY:
        {
            _State->_EffectCategoryId = (EyeCandy::Category) SelectedIndex;

            const auto Category = EffectCategories[(size_t) _State->_EffectCategoryId];

            _State->_EffectId = Category.Effects[0].Id;

            UpdateEffect();

            ChangedSettings = ConfigurationChanges::All;
            break;
        }

        case IDC_EFFECT:
        {
            const auto Category = EffectCategories[(size_t) _State->_EffectCategoryId];

            _State->_EffectId = Category.Effects[(size_t) SelectedIndex].Id;

            ChangedSettings = ConfigurationChanges::All;
            break;
        }

        case IDC_EFFECT_QUALITY:
        {
            _State->_EffectQuality = (EyeCandy::Quality) SelectedIndex;
            break;
        }

        case IDC_EFFECT_SPEED:
        {
            _State->_EffectSpeed = (EyeCandy::Speed) SelectedIndex;
            break;
        }

        case IDC_EFFECT_BRIGHTNESS:
        {
            _State->_EffectBrightness = (EyeCandy::Brightness) SelectedIndex;
            break;
        }

        case IDC_EFFECT_DENSITY:
        {
            _State->_EffectDensity = (EyeCandy::Density) SelectedIndex;
            break;
        }
    }

    NotifyParent(ChangedSettings);
}

/// <summary>
/// Handles the notification when the content of an edit control has been changed.
/// </summary>
void eye_candy_page_t::OnEditChange(UINT code, int id, CWindow) noexcept
{
/*
    if ((_State == nullptr) || _IgnoreNotifications || (code != EN_CHANGE))
        return;

    WCHAR Text[MAX_PATH] = { };

    GetDlgItemTextW(id, Text, _countof(Text));

    switch (id)
    {
        default:
            return;

        case IDC_PRESET_NAME:
        {
            UpdateControls();

            return;
        }
    }
*/
}

/// <summary>
/// Handles the notification when a control loses focus.
/// </summary>
void eye_candy_page_t::OnEditLostFocus(UINT code, int id, CWindow) noexcept
{
/*
    if ((_State == nullptr) || _IgnoreNotifications)
        return;

    if (id == IDC_PRESETS_ROOT)
        FillPresetListBox();
*/
}

/// <summary>
/// Handles the notification when a button is clicked.
/// </summary>
void eye_candy_page_t::OnButtonClick(UINT, int id, CWindow) noexcept
{
/*
    if (_State == nullptr)
        return;

    switch (id)
    {
        default:
            return;

        case IDC_PRESET_DELETE:
        {
            WCHAR PresetName[MAX_PATH] = { };

            GetDlgItemTextW(IDC_PRESET_NAME, PresetName, _countof(PresetName));

            PresetManager::Delete(_State->_PresetsDirectoryPath, PresetName);

            FillPresetListBox();
            break;
        }
    }
*/
}

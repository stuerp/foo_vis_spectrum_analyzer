
/** $VER: PresetsPage.h (2026.02.22) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class presets_page_t : public CDialogResize<presets_page_t>, public page_t
{
public:
    presets_page_t(int id) : page_t(id) { }

    presets_page_t(const presets_page_t &) = delete;
    presets_page_t & operator=(const presets_page_t &) = delete;
    presets_page_t(presets_page_t &&) = delete;
    presets_page_t & operator=(presets_page_t &&) = delete;

    virtual ~presets_page_t() noexcept { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;
    void OnDoubleClick(UINT, int, CWindow) noexcept override final;

    BEGIN_MSG_MAP(presets_page_t)
        CHAIN_MSG_MAP(page_t)
        CHAIN_MSG_MAP(CDialogResize<presets_page_t>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(presets_page_t)
        DLGRESIZE_CONTROL(IDC_PRESET_NAMES, DLSZ_SIZE_Y)

        DLGRESIZE_CONTROL(IDC_PRESET_NAME_LBL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_PRESET_NAME, DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDC_PRESET_LOAD, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_PRESET_SAVE, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_PRESET_DELETE, DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

private:
    void InitializeControls() noexcept override;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept override { };

    void GetPresetNames() noexcept;
    void GetPreset(const std::wstring & presetName) noexcept;

private:
    std::vector<std::wstring> _PresetNames;
};

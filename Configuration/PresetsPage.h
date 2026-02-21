
/** $VER: PresetsPage.h (2026.02.21) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class presets_page_t : public page_t
{
public:
    presets_page_t(int id) : page_t(id) { }

    presets_page_t(const presets_page_t &) = delete;
    presets_page_t & operator=(const presets_page_t &) = delete;
    presets_page_t(presets_page_t &&) = delete;
    presets_page_t & operator=(presets_page_t &&) = delete;

    virtual ~presets_page_t() { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;
    void OnDoubleClick(UINT, int, CWindow) noexcept override final;

private:
    void InitializeControls() noexcept override;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept override { };

    void GetPresetNames() noexcept;
    void GetPreset(const std::wstring & presetName) noexcept;

private:
    std::vector<std::wstring> _PresetNames;
};

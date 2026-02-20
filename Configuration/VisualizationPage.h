
/** $VER: VisualizationPage.h (2026.02.20) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class visualization_page_t : public page_t
{
public:
    visualization_page_t(int id) : page_t(id) { }

    visualization_page_t(const visualization_page_t &) = delete;
    visualization_page_t & operator=(const visualization_page_t &) = delete;
    visualization_page_t(visualization_page_t &&) = delete;
    visualization_page_t & operator=(visualization_page_t &&) = delete;

    virtual ~visualization_page_t() { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;

private:
    void InitializeControls() noexcept;
    void UpdateControls() noexcept override;
    void Terminate() noexcept;

private:
    std::vector<VisualElement> _ActiveStyles;   // The styles that are relevant for the current visualization.

    size_t _SelectedStyle;                      // Index of the selected style in the listbox.
};

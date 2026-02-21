
/** $VER: StylesPage.h (2026.02.21) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class styles_page_t : public page_t
{
public:
    styles_page_t(int id) : page_t(id) { }

    styles_page_t(const styles_page_t &) = delete;
    styles_page_t & operator=(const styles_page_t &) = delete;
    styles_page_t(styles_page_t &&) = delete;
    styles_page_t & operator=(styles_page_t &&) = delete;

    virtual ~styles_page_t() { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;
    void OnDoubleClick(UINT code, int id, CWindow) noexcept final;

    LRESULT OnChanged(LPNMHDR nmhd) noexcept override final;

private:
    void InitializeControls() noexcept;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept;

    void InitializeStyles() noexcept;
    void UpdateColorControls() noexcept;
    void UpdateCurrentColor(style_t * style) const noexcept;
    void UpdateGradientStopPositons(style_t * style, size_t index) const noexcept;

private:
    std::vector<std::shared_ptr<CNumericEdit>> _NumericEdits;
    std::vector<VisualElement> _ActiveStyles;   // The styles relevant to the current visualization.
    size_t _SelectedStyle;                      // Index of the selected style in the listbox.

    CColorButton _Color;
    CColorButton _Gradient;
    CColorListBox _Colors;
};

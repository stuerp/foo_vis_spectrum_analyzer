
/** $VER: FrequenciesPage.h (2026.02.21) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class frequencies_page_t : public page_t
{
public:
    frequencies_page_t(int id) : page_t(id) { }

    frequencies_page_t(const frequencies_page_t &) = delete;
    frequencies_page_t & operator=(const frequencies_page_t &) = delete;
    frequencies_page_t(frequencies_page_t &&) = delete;
    frequencies_page_t & operator=(frequencies_page_t &&) = delete;

    virtual ~frequencies_page_t() { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;

    LRESULT OnDeltaPos(LPNMHDR nmhd) noexcept;

private:
    void InitializeControls() noexcept override;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept override;

private:
    std::vector<std::shared_ptr<CNumericEdit>> _NumericEdits;
};


/** $VER: FiltersPage.h (2026.02.22) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class filters_page_t : public page_t
{
public:
    filters_page_t(int id) : page_t(id) { }

    filters_page_t(const filters_page_t &) = delete;
    filters_page_t & operator=(const filters_page_t &) = delete;
    filters_page_t(filters_page_t &&) = delete;
    filters_page_t & operator=(filters_page_t &&) = delete;

    virtual ~filters_page_t() noexcept { }

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

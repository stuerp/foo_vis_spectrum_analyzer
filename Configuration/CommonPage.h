
/** $VER: CommonPage.h (2026.03.07) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class common_page_t : public page_t
{
public:
    common_page_t(int id) : page_t(id) { }

    common_page_t(const common_page_t &) = delete;
    common_page_t & operator=(const common_page_t &) = delete;
    common_page_t(common_page_t &&) = delete;
    common_page_t & operator=(common_page_t &&) = delete;

    virtual ~common_page_t() noexcept { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int id, CWindow) noexcept override final;

    LRESULT OnDeltaPos(LPNMHDR nmhd) noexcept;

private:
    void InitializeControls() noexcept override;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept override;

private:
    std::vector<std::shared_ptr<CNumericEdit>> _NumericEdits;
};

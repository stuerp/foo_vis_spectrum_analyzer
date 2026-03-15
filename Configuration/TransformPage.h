
/** $VER: TransformPage.h (2026.02.22) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class transform_page_t : public page_t
{
public:
    transform_page_t(int id) : page_t(id) { }

    transform_page_t(const transform_page_t &) = delete;
    transform_page_t & operator=(const transform_page_t &) = delete;
    transform_page_t(transform_page_t &&) = delete;
    transform_page_t & operator=(transform_page_t &&) = delete;

    virtual ~transform_page_t() noexcept { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;

    LRESULT OnDeltaPos(LPNMHDR nmhd) noexcept;

private:
    void InitializeControls() noexcept override;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept override;

private:
    std::vector<std::shared_ptr<CNumericEdit>> _NumericEdits;
};

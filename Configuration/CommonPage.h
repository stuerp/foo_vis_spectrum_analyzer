
/** $VER: CommonPage.h (2026.06.08) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include "Page.h"

class common_page_t final : public page_t
{
public:
    common_page_t(int id) : page_t(id) { }

    common_page_t(const common_page_t &) = delete;
    common_page_t & operator=(const common_page_t &) = delete;
    common_page_t(common_page_t &&) = delete;
    common_page_t & operator=(common_page_t &&) = delete;

    virtual ~common_page_t() = default;

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int id, CWindow) noexcept override final;

    LRESULT OnDeltaPos(LPNMHDR nmh) noexcept;
    LRESULT OnHScroll(UINT, WPARAM, LPARAM) noexcept;

private:
    void InitializeControls() noexcept override final;
    void UpdateControls() noexcept override final;
    void TerminateControls() noexcept override final;
};

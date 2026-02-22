
/** $VER: GraphsPage.h (2026.02.22) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include <sdk/coreDarkMode.h>
#include <sdk/cfg_var.h>

#include "Page.h"
#include "Resources.h"
#include "State.h"

class graphs_page_t : public page_t
{
public:
    graphs_page_t(int id) : page_t(id) { }

    graphs_page_t(const graphs_page_t &) = delete;
    graphs_page_t & operator=(const graphs_page_t &) = delete;
    graphs_page_t(graphs_page_t &&) = delete;
    graphs_page_t & operator=(graphs_page_t &&) = delete;

    virtual ~graphs_page_t() noexcept { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;

    LRESULT OnDeltaPos(LPNMHDR nmhd) noexcept override final;

private:
    void InitializeControls() noexcept override;
    void UpdateControls() noexcept override;
    void TerminateControls() noexcept override;

    void InitializeXAxisMode() noexcept;
    void InitializeYAxisMode() noexcept;

    void UpdateSelectedChannels() noexcept;

private:
    std::vector<std::shared_ptr<CNumericEdit>> _NumericEdits;

    size_t _SelectedGraph;                      // Index of the selected graph in the listbox.
};

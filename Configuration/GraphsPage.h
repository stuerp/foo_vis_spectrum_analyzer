
/** $VER: GraphsPage.h (2026.10.03) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include "Page.h"

class graphs_page_t final : public page_t
{
public:
    graphs_page_t(int id) : page_t(id) { }

    graphs_page_t(const graphs_page_t &) = delete;
    graphs_page_t & operator=(const graphs_page_t &) = delete;
    graphs_page_t(graphs_page_t &&) = delete;
    graphs_page_t & operator=(graphs_page_t &&) = delete;

    virtual ~graphs_page_t() noexcept = default;

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;

    LRESULT OnDeltaPos(LPNMHDR nmh) noexcept override final;

    LRESULT OnChannelChanged(int, LPNMHDR nmh, BOOL &) noexcept;

    BEGIN_MSG_MAP(graphs_page_t)
        NOTIFY_HANDLER(IDC_CHANNELS, LVN_ITEMCHANGED, OnChannelChanged);

        CHAIN_MSG_MAP(page_t)
    END_MSG_MAP()

private:
    void InitializeControls() noexcept override final;
    void UpdateControls() noexcept override final;
    void TerminateControls() noexcept override final;

    void InitializeXAxisMode() noexcept;
    void InitializeYAxisMode() noexcept;

    void UpdateActiveChannelMask() noexcept;

    static void SwapItems(CListViewCtrl & w, int SrcIndex, int DstIndex) noexcept;

private:
    size_t _SelectedGraph;                      // Index of the selected graph in the listbox.
    CFont _SymbolFont;
};

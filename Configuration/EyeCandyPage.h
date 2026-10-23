
/** $VER: EyeCandyPage.h (2026.10.21) P. Stuer - Declares a configuration dialog page. **/

#pragma once

#include "pch.h"

#include "Page.h"
#include "Resources.h"

class eye_candy_page_t : public CDialogResize<eye_candy_page_t>, public page_t
{
public:
    eye_candy_page_t(int id) : page_t(id) { }

    eye_candy_page_t(const eye_candy_page_t &) = delete;
    eye_candy_page_t & operator=(const eye_candy_page_t &) = delete;
    eye_candy_page_t(eye_candy_page_t &&) = delete;
    eye_candy_page_t & operator=(eye_candy_page_t &&) = delete;

    virtual ~eye_candy_page_t() noexcept { }

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept override final;

    void OnSelectionChanged(UINT, int, CWindow) noexcept override final;
    void OnEditChange(UINT, int, CWindow) noexcept override final;
    void OnEditLostFocus(UINT code, int id, CWindow) noexcept override final;
    void OnButtonClick(UINT, int, CWindow) noexcept override final;

    BEGIN_MSG_MAP(eye_candy_page_t)
        CHAIN_MSG_MAP(page_t)
        CHAIN_MSG_MAP(CDialogResize<eye_candy_page_t>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(eye_candy_page_t)
/*
        DLGRESIZE_CONTROL(IDC_PROPERTIES, DLSZ_SIZE_Y)

        DLGRESIZE_CONTROL(IDC_PRESET_NAME_LBL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_PRESET_NAME, DLSZ_MOVE_Y)

        DLGRESIZE_CONTROL(IDC_PRESET_LOAD, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_PRESET_SAVE, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_PRESET_DELETE, DLSZ_MOVE_Y)
*/
    END_DLGRESIZE_MAP()

private:
    void InitializeControls() noexcept override;
    void UpdateControls() noexcept override { };
    void TerminateControls() noexcept override { };

    void UpdateEffect() noexcept;
};

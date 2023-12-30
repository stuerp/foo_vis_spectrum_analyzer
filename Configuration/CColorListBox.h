
/** $VER: CColorListBox.h (2023.11.26) P. Stuer - Implements a list box that displays colors using WTL. **/

#pragma once

#include "framework.h"
#include "Support.h"

#include <vector>

#include "CDirectXControl.h"
#include "COwnerDrawnListBox.h"

class CColorListBox : public COwnerDrawnListBox<CColorListBox>, public CDirectXControl
{
public:
    BEGIN_MSG_MAP(CColorListBox)
        MSG_WM_SIZE(OnSize)

        REFLECTED_COMMAND_CODE_HANDLER(LBN_DBLCLK, OnDblClick)

        CHAIN_MSG_MAP(COwnerDrawnListBox<CColorListBox>)
    END_MSG_MAP()

    CColorListBox() { }

    CColorListBox(const CColorListBox &) = delete;
    CColorListBox & operator=(const CColorListBox &) = delete;
    CColorListBox(CColorListBox &&) = delete;
    CColorListBox & operator=(CColorListBox &&) = delete;

    virtual ~CColorListBox() { }

    void Initialize(HWND hWnd);
    void Terminate();

    void GetColors(std::vector<D2D1_COLOR_F> & colors) const;
    void SetColors(const std::vector<D2D1_COLOR_F> & colors);

    void DrawItem(LPDRAWITEMSTRUCT dis);
    void MeasureItem(LPMEASUREITEMSTRUCT mis);

private:
    LRESULT OnDblClick(WORD, WORD, HWND, BOOL & handled);

    void SendChangedNotification() const noexcept;

private:
    #pragma region DirectX
    HRESULT CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size) override;
    void ReleaseDeviceSpecificResources() override;
    #pragma endregion

private:
    #pragma region DirectX
    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _SolidBrush;
    #pragma endregion

private:
    std::vector<D2D1_COLOR_F> _Colors;
};

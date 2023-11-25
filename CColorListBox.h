
/** $VER: CColorListBox.h (2023.11.25) P. Stuer - Implements a list box that displays colors using WTL. **/

#pragma once

#include "framework.h"

#include <vector>

#include "CDirectXControl.h"
#include "COwnerDrawnListBox.h"
#include "Support.h"

class CColorListBox : public COwnerDrawnListBox<CColorListBox>, public CDirectXControl
{
public:
    BEGIN_MSG_MAP(CColorListBox)
        MSG_WM_SIZE(OnSize)

        REFLECTED_COMMAND_CODE_HANDLER(LBN_DBLCLK, OnDblClick)

        CHAIN_MSG_MAP(COwnerDrawnListBox<CColorListBox>)
    END_MSG_MAP()

    void Initialize(HWND hWnd);

    void GetColors(std::vector<D2D1_COLOR_F> & colors) const;
    void SetColors(const std::vector<D2D1_COLOR_F> & colors);

    bool Add(const D2D1_COLOR_F & Color);
    bool Remove();

    void DrawItem(LPDRAWITEMSTRUCT dis);
    void MeasureItem(LPMEASUREITEMSTRUCT mis);

private:
    LRESULT OnDblClick(WORD, WORD, HWND, BOOL & handled);

    void UpdateItems();
    void SendColorsChangedNotification() const noexcept;

private:
    #pragma region DirectX
    HRESULT CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size) override;
    void ReleaseDeviceSpecificResources() override;
    #pragma endregion

private:
    #pragma region DirectX
    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _SolidBrush;
    D2D1_COLOR_F _Color;
    #pragma endregion

private:
    std::vector<D2D1_COLOR_F> _Colors;
};

#define NM_COLORS_CHANGED (NM_RETURN)

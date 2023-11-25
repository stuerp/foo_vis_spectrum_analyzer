
/** $VER: CColorButton.h (2023.11.25) P. Stuer **/

#pragma once

#include "framework.h"

#include <vector>

#include "CDirectXControl.h"

/// <summary>
/// Implements a color control that can display gradients.
/// </summary>
class CColorButton : public CWindowImpl<CColorButton>, public CDirectXControl
{
public:
    DECLARE_WND_CLASS_EX(NULL, 0, COLOR_3DFACE)

    BEGIN_MSG_MAP(CColorButton)
        MSG_WM_SIZE(OnSize)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClick)
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMessage)
    END_MSG_MAP()

    CColorButton() : _Color() { }

    void Initialize(HWND hWnd);
    void SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops);
    void SetColor(COLORREF color);
    void SetColor(const D2D1_COLOR_F & color);

private:
    void OnPaint(HDC);
    LRESULT OnLButtonDblClick(UINT, CPoint);
    LRESULT OnMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam, BOOL & handled);

private:
    #pragma region DirectX
    HRESULT CreateDeviceSpecificResources(HWND hWnd, D2D1_SIZE_U size) override;
    void ReleaseDeviceSpecificResources() override;
    #pragma endregion

private:
    #pragma region DirectX
    // Device-specific resources
    CComPtr<ID2D1Brush> _Brush;
    D2D1_COLOR_F _Color;
    #pragma endregion

private:
    CToolTipCtrl _ToolTip;

    std::vector<D2D1_GRADIENT_STOP> _GradientStops;
};

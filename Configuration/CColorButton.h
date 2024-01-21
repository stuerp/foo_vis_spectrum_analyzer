
/** $VER: CColorButton.h (2024.01.16) P. Stuer **/

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
    CColorButton() : _Color() { }

    CColorButton(const CColorButton &) = delete;
    CColorButton & operator=(const CColorButton &) = delete;
    CColorButton(CColorButton &&) = delete;
    CColorButton & operator=(CColorButton &&) = delete;

    virtual ~CColorButton() { }

    void Initialize(HWND hWnd);
    void Terminate();

    void SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops);

    void GetColor(D2D1_COLOR_F & color) const;
    void SetColor(const D2D1_COLOR_F & color);
    void SetColor(COLORREF color);

private:
    void OnPaint(HDC);
    LRESULT OnLButtonDown(UINT, CPoint);

    void SendChangedNotification() const noexcept;

    DECLARE_WND_CLASS_EX(NULL, 0, COLOR_3DFACE)

    BEGIN_MSG_MAP(CColorButton)
        MSG_WM_SIZE(OnSize)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
    END_MSG_MAP()

private:
    #pragma region DirectX
    HRESULT CreateDeviceSpecificResources() override;
    HRESULT CreatePatternBrush(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources() override;
    #pragma endregion

private:
    #pragma region DirectX
    // Device-specific resources
    CComPtr<ID2D1Brush> _Brush;
    CComPtr<ID2D1BitmapBrush> _PatternBrush;
    #pragma endregion

private:
    D2D1_COLOR_F _Color;
    std::vector<D2D1_GRADIENT_STOP> _GradientStops;
};

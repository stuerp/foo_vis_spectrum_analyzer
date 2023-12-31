
/** $VER: CColorDialogEx.h (2023.12.31) P. Stuer - Implements a color dialog with alpha channel support using WTL. **/

#pragma once

#include "framework.h"

#include "CColorButton.h"

class CColorDialogEx
{
public:
    CColorDialogEx() { }

    CColorDialogEx(const CColorDialogEx &) = delete;
    CColorDialogEx & operator=(const CColorDialogEx &) = delete;
    CColorDialogEx(CColorDialogEx &&) = delete;
    CColorDialogEx & operator=(CColorDialogEx &&) = delete;

    virtual ~CColorDialogEx() { }

    bool SelectColor(HWND hWnd, D2D1_COLOR_F & color);

private:
    static UINT_PTR CALLBACK Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    UINT_PTR ProcessMessage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawAlphaSliderCursor(HWND hDlg);
    void UpdateAlphaSlider();

    // Explicit version to prevent truncation.
    LONG MapEx(float value, float srcMin, float srcMax, LONG dstMin, LONG dstMax)
    {
        return dstMin + (LONG) ((value - srcMin) * (float) (dstMax - dstMin) / (srcMax - srcMin));
    }

private:
    D2D1_COLOR_F _Color;

    CColorButton _Alpha;
    RECT _SliderRect;

    const LONG ArrowSize = 6;
};


/** $VER: color_dialog_t.h (2026.09.26) P. Stuer - Implements a color dialog with alpha channel support using WTL. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "ColorButton.h"

class color_dialog_t
{
public:
    color_dialog_t() { }

    color_dialog_t(const color_dialog_t &) = delete;
    color_dialog_t & operator=(const color_dialog_t &) = delete;
    color_dialog_t(color_dialog_t &&) = delete;
    color_dialog_t & operator=(color_dialog_t &&) = delete;

    virtual ~color_dialog_t() noexcept { }

    bool SelectColor(HWND hWnd, D2D1_COLOR_F & color) noexcept;

private:
    static UINT_PTR CALLBACK Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

    UINT_PTR ProcessMessage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    void DrawAlphaSliderCursor(HWND hDlg) noexcept;
    void UpdateAlphaSlider() noexcept;

    // Explicit version to prevent truncation.
    static LONG MapEx(float value, float srcMin, float srcMax, LONG dstMin, LONG dstMax) noexcept
    {
        return dstMin + (LONG) ((value - srcMin) * (float) (dstMax - dstMin) / (srcMax - srcMin));
    }

private:
    D2D1_COLOR_F _Color;

    color_button_t _Alpha;
    RECT _SliderRect;

    const LONG ArrowSize = 6;
};

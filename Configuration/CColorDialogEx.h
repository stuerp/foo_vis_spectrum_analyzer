
/** $VER: CColorDialogEx.h (2024.03.09) P. Stuer - Implements a color dialog with alpha channel support using WTL. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <atlbase.h>
#include <atltypes.h>
#include <atlstr.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>
#include <atlcom.h>
#include <atlcrack.h>

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

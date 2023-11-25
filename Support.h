
/** $VER: Support.h (2023.11.25) P. Stuer **/

#pragma once

#include "framework.h"

inline D2D1_COLOR_F ToD2D1_COLOR_F(COLORREF color)
{
    return  D2D1::ColorF(GetRValue(color) / 255.f, GetGValue(color) / 255.f, GetBValue(color) / 255.f);
}

inline COLORREF ToCOLORREF(const D2D1_COLOR_F & color)
{
    return RGB((BYTE)(color.r * 255.f), (BYTE)(color.g * 255.f), (BYTE)(color.b * 255.f));
}

static bool SelectColor(HWND hWnd, D2D1_COLOR_F & color)
{
    static COLORREF CustomColors[16] =
    {
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
    };

    CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };

    cc.hwndOwner = hWnd;

    cc.lpCustColors = (LPDWORD) CustomColors;
    cc.rgbResult = ToCOLORREF(color);
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;

    if (!::ChooseColorW(&cc))
        return false;

    color = ToD2D1_COLOR_F(cc.rgbResult);

    return true;
}

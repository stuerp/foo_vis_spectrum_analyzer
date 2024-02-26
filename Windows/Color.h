
/** $VER: Color.h (2024.02.26) P. Stuer - Color support **/

#pragma once

#include "framework.h"

class Color
{
public:
    /// <summary>
    /// Blends two colors using the specified factor.
    /// </summary>
    inline static COLORREF Blend(COLORREF c1, COLORREF c2, int factor)
    {
        return RGB(GetRValue(c1) + ((GetRValue(c2) - GetRValue(c1)) * factor / 100L),
                   GetGValue(c1) + ((GetGValue(c2) - GetGValue(c1)) * factor / 100L),
                   GetBValue(c1) + ((GetBValue(c2) - GetBValue(c1)) * factor / 100L));
    }

    /// <summary>
    /// Converts a COLORREF to a D2D1_COLOR_F.
    /// </summary>
    inline static D2D1_COLOR_F ToD2D1_COLOR_F(COLORREF color)
    {
        return  D2D1::ColorF(GetRValue(color) / 255.f, GetGValue(color) / 255.f, GetBValue(color) / 255.f);
    }

    /// <summary>
    /// Converts a D2D1_COLOR_F to a COLORREF.
    /// </summary>
    inline static COLORREF ToCOLORREF(const D2D1_COLOR_F & color)
    {
        return RGB((BYTE)(color.r * 255.f), (BYTE)(color.g * 255.f), (BYTE)(color.b * 255.f));
    }

    /// <summary>
    /// Creates a GDI brush from the specified DirectX color.
    /// </summary>
    inline static HBRUSH CreateBrush(const D2D1_COLOR_F & color)
    {
        return ::CreateSolidBrush(ToCOLORREF(color));
    }
};


/** $VER: Color.h (2024.01.21) P. Stuer - Color support **/

#pragma once

#include "framework.h"

class Color
{
public:
    inline static COLORREF Blend(COLORREF c1, COLORREF c2, int factor)
    {
        return RGB(GetRValue(c1) + ((GetRValue(c2) - GetRValue(c1)) * factor / 100L),
                   GetGValue(c1) + ((GetGValue(c2) - GetGValue(c1)) * factor / 100L),
                   GetBValue(c1) + ((GetBValue(c2) - GetBValue(c1)) * factor / 100L));
    }
};

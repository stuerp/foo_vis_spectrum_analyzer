
/** $VER: DominantColors.h (2024.01.07) P. Stuer - Determines the dominant colors of an image. Based on BetterDominantColors (https://github.com/Humeur/better-dominant-colors) **/

#pragma once

#include "framework.h"

class DominantColors
{
public:
    HRESULT Get(CComPtr<IWICBitmapFrameDecode> frame, size_t count, std::vector<D2D1_COLOR_F> & colors) const noexcept;

private:
};

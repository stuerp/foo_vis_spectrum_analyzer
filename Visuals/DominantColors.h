
/** $VER: DominantColors.h (2024.01.07) P. Stuer - Determines the dominant colors of an image. Based on BetterDominantColors (https://github.com/Humeur/better-dominant-colors) **/

#pragma once

#include "framework.h"

class DominantColors
{
public:
    HRESULT Get(CComPtr<IWICBitmapFrameDecode> frame, size_t count, std::vector<uint32_t> & colors) const noexcept;
};

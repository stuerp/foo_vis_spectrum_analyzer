
/** $VER: ColorThief.h (2024.01.15) P. Stuer **/

#pragma once

#include "framework.h"

#include <stdint.h>

#include <array>
#include <vector>

namespace ColorThief
{

using color_t = std::array<uint8_t, 3>;

static const uint32_t DefaultColorCount = 5;
static const uint32_t DefaultQuality = 10;
static const bool DefaultIgnoreBright = true;

HRESULT GetPalette(IWICBitmapSource * bitmapSource, std::vector<color_t> & palette, uint32_t colorCount = DefaultColorCount, uint32_t quality = DefaultQuality, bool ignoreWhite = DefaultIgnoreBright);
HRESULT GetDominantColor(IWICBitmapSource * bitmapSource, color_t & color, uint32_t quality = DefaultQuality, bool ignoreWhite = DefaultIgnoreBright);

}

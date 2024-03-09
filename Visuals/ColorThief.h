
/** $VER: ColorThief.h (2024.03.09) P. Stuer - Based on Fast ColorThief, https://github.com/bedapisl/fast-colorthief **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <windows.h>
#include <wincodec.h>

#include <stdint.h>

#include <array>
#include <vector>

namespace ColorThief
{

using color_t = std::array<uint8_t, 3>;

static const uint32_t DefaultColorCount = 5;
static const uint32_t DefaultQuality = 10;
static const bool DefaultIgnoreLightColors = true;
static const uint8_t DefaultLightnessThreshold = 250;
static const uint8_t DefaultTransparencyThreshold = 125;

HRESULT GetPalette(IWICBitmapSource * bitmapSource, std::vector<color_t> & palette, uint32_t colorCount = DefaultColorCount, uint32_t quality = DefaultQuality, bool ignoreLightColors = DefaultIgnoreLightColors, uint8_t lightnessThreshold = DefaultLightnessThreshold, uint8_t transparencyThreshold = DefaultTransparencyThreshold);
HRESULT GetDominantColor(IWICBitmapSource * bitmapSource, color_t & color, uint32_t quality = DefaultQuality, bool ignoreLightColors = DefaultIgnoreLightColors, uint8_t lightnessThreshold = DefaultLightnessThreshold, uint8_t transparencyThreshold = DefaultTransparencyThreshold);
}

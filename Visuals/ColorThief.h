
/** $VER: ColorThief.h (2026.09.30) P. Stuer - Based on Fast ColorThief, https://github.com/bedapisl/fast-colorthief **/

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

static constexpr uint32_t DefaultColorCount = 5;
static constexpr uint32_t DefaultQuality = 10;
static constexpr bool DefaultIgnoreLightColors = true;
static constexpr uint8_t DefaultLightnessThreshold = 250;
static constexpr uint8_t DefaultTransparencyThreshold = 125;

HRESULT GetPalette      (IWICBitmapSource * bitmapSource, uint32_t colorCount, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparencyThreshold, std::vector<color_t> & palette);
HRESULT GetDominantColor(IWICBitmapSource * bitmapSource, uint32_t quality, bool ignoreLightColors, uint8_t lightnessThreshold, uint8_t transparencyThreshold, color_t & color);
}

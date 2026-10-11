
/** $VER: Color.cpp (2026.10.11) P. Stuer - Color support **/

#include "pch.h"

#include "Color.h"

#include <algorithm>
#include <cmath>
#include <vector>

struct hsl_t
{
    FLOAT H;
    FLOAT S;
    FLOAT L;
    FLOAT A;
};

hsl_t RGB2HSL(const D2D1_COLOR_F & rgb) noexcept;
D2D1_COLOR_F HSL2RGB(const hsl_t & hsl) noexcept;
FLOAT EvalHSL(FLOAT x, FLOAT y, FLOAT z) noexcept;

/// <summary>
/// Sorts a color vector by hue, then lightness, then saturation.
/// </summary>
void color_t::SortColorsByHue(std::vector<D2D1_COLOR_F> & colors, bool ascending) noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right) noexcept -> bool
    {
        const hsl_t lhs = RGB2HSL(left);
        const hsl_t rhs = RGB2HSL(right);

        if (lhs.H != rhs.H)
            return ascending ? lhs.H < rhs.H : lhs.H > rhs.H;

        if (lhs.L != rhs.L)
            return ascending ? lhs.L < rhs.L : lhs.L > rhs.L;

        if (lhs.S != rhs.S)
            return ascending ? lhs.S < rhs.S : lhs.S > rhs.S;

        return false;
    });
}

/// <summary>
/// Sorts a color vector by saturation, then hue, then lightness.
/// </summary>
void color_t::SortColorsBySaturation(std::vector<D2D1_COLOR_F> & colors, bool ascending) noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right) noexcept -> bool
    {
        const hsl_t lhs = RGB2HSL(left);
        const hsl_t rhs = RGB2HSL(right);

        if (lhs.S != rhs.S)
            return ascending ? lhs.S < rhs.S : lhs.S > rhs.S;

        if (lhs.H != rhs.H)
            return ascending ? lhs.H < rhs.H : lhs.H > rhs.H;

        if (lhs.L != rhs.L)
            return ascending ? lhs.L < rhs.L : lhs.L > rhs.L;

        return false;
    });
}

/// <summary>
/// Sorts a color vector by lightness, then hue, then saturation.
/// </summary>
void color_t::SortColorsByLightness(std::vector<D2D1_COLOR_F> & colors, bool ascending) noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right) noexcept -> bool
    {
        const hsl_t lhs = RGB2HSL(left);
        const hsl_t rhs = RGB2HSL(right);

        if (lhs.L != rhs.L)
            return ascending ? lhs.L < rhs.L : lhs.L > rhs.L;

        if (lhs.H != rhs.H)
            return ascending ? lhs.H < rhs.H : lhs.H > rhs.H;

        if (lhs.S != rhs.S)
            return ascending ? lhs.S < rhs.S : lhs.S > rhs.S;

        return false;
    });
}

/// <summary>
/// Converts an RGB color to HSL.
/// </summary>
hsl_t RGB2HSL(const D2D1_COLOR_F & rgb) noexcept
{
    const FLOAT MaxC = std::max({ rgb.r, rgb.g, rgb.b });
    const FLOAT MinC = std::min({ rgb.r, rgb.g, rgb.b });
    const FLOAT Delta = MaxC - MinC;

    hsl_t HSL = {};

    // Lightness must be calculated before saturation.
    HSL.L = (MaxC + MinC) * 0.5f;
    HSL.A = rgb.a;

    if (Delta == 0.f)
    {
        // Achromatic color. Hue and saturation remain zero.
        return HSL;
    }

    if (MaxC == rgb.r)
    {
        HSL.H = (rgb.g - rgb.b) / Delta;

        if (rgb.g < rgb.b)
            HSL.H += 6.f;
    }
    else
    if (MaxC == rgb.g)
    {
        HSL.H = (rgb.b - rgb.r) / Delta + 2.f;
    }
    else
    {
        HSL.H = (rgb.r - rgb.g) / Delta + 4.f;
    }

    HSL.H /= 6.f;

    HSL.S = (HSL.L > 0.5f) ? Delta / (2.f - MaxC - MinC) : Delta / (MaxC + MinC);

    return HSL;
}

/// <summary>
/// Converts a HSL color to RGB.
/// </summary>
D2D1_COLOR_F HSL2RGB(const hsl_t & hsl) noexcept
{
    if (hsl.S == 0.f)
        return D2D1::ColorF(hsl.L, hsl.L, hsl.L, hsl.A); // Achromatic

    const FLOAT q = (hsl.L < 0.5f) ? hsl.L * (1.f + hsl.S) : hsl.L + hsl.S - hsl.L * hsl.S;
    const FLOAT p = 2.f * hsl.L - q;

    return D2D1::ColorF(EvalHSL(p, q, hsl.H + 1.f / 3.f), EvalHSL(p, q, hsl.H), EvalHSL(p, q, hsl.H - 1.f / 3.f), hsl.A);
}

/// <summary>
/// Evaluates one RGB component from normalized HSL values.
/// </summary>
FLOAT EvalHSL(FLOAT p, FLOAT q, FLOAT hue) noexcept
{
    hue -= std::floor(hue);

    if (hue < 1.f / 6.f)
        return p + (q - p) * 6.f * hue;

    if (hue < 1.f / 2.f)
        return q;

    if (hue < 2.f / 3.f)
        return p + (q - p) * (2.f / 3.f - hue) * 6.f;

    return p;
}

/// <summary>
/// The coefficients account for human vision being much more sensitive to green than blue. The sRGB channels must be linearized before applying those weights.
/// </summary>
constexpr float GetPerceivedLuminance(const D2D1_COLOR_F & color) noexcept
{
    // Relative luminance, https://www.w3.org/WAI/GL/wiki/Relative_luminance
    constexpr auto sRGBToLinear = [](float value) noexcept -> float
    {
        return (value <= 0.04045f) ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
    };

    const float r = sRGBToLinear(color.r);
    const float g = sRGBToLinear(color.g);
    const float b = sRGBToLinear(color.b);

    return (0.2126f * r) + (0.7152f * g) + (0.0722f * b);
}

/// <summary>
/// Sorts a color vector from dark to light by relative luminence.
/// </summary>
void color_t::SortColorsDarkToLight(std::vector<D2D1_COLOR_F> & colors, bool ascending) noexcept
{
    std::ranges::sort(colors, std::ranges::less{}, GetPerceivedLuminance);

    if (!ascending)
        std::reverse(colors.begin(), colors.end());
}

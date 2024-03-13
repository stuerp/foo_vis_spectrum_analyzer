
/** $VER: Color.cpp (2024.01.21) P. Stuer - Color support **/

#include "Color.h"

#include <algorithm>

struct ColorHSL
{
    FLOAT H;
    FLOAT S;
    FLOAT L;
};

ColorHSL RGB2HSL(const D2D1_COLOR_F & rgb);
D2D1_COLOR_F HSL2RGB(const ColorHSL & hsl);
FLOAT EvalHSL(FLOAT x, FLOAT y, FLOAT z);

/// <summary>
/// Sorts a color vector by hue.
/// </summary>
void Color::SortColorsByHue(std::vector<D2D1_COLOR_F> & colors, bool ascending) noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right)
    {
        ColorHSL Left = RGB2HSL(left);
        ColorHSL Right = RGB2HSL(right);

        if (Left.H != Right.H)
            return ascending ? Left.H < Right.H : Left.H > Right.H;

        if (Left.S != Right.S)
            return ascending ? Left.S < Right.S : Left.S > Right.S;

        if (Left.L != Right.L)
            return ascending ? Left.L < Right.L : Left.L > Right.L;

        return false;
    });
}

/// <summary>
/// Sorts a color vector by saturation.
/// </summary>
void Color::SortColorsBySaturation(std::vector<D2D1_COLOR_F> & colors, bool ascending) noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right)
    {
        ColorHSL Left = RGB2HSL(left);
        ColorHSL Right = RGB2HSL(right);

        if (Left.S != Right.S)
            return ascending ? Left.S < Right.S : Left.S > Right.S;

        if (Left.H != Right.H)
            return ascending ? Left.H < Right.H : Left.H > Right.H;

        if (Left.L != Right.L)
            return ascending ? Left.L < Right.L : Left.L > Right.L;

        return false;
    });
}

/// <summary>
/// Sorts a color vector by lightness.
/// </summary>
void Color::SortColorsByLightness(std::vector<D2D1_COLOR_F> & colors, bool ascending) noexcept
{
    std::sort(colors.begin(), colors.end(), [ascending](const D2D1_COLOR_F & left, const D2D1_COLOR_F & right)
    {
        ColorHSL Left = RGB2HSL(left);
        ColorHSL Right = RGB2HSL(right);

        if (Left.L != Right.L)
            return ascending ? Left.L < Right.L : Left.L > Right.L;

        if (Left.H != Right.H)
            return ascending ? Left.H < Right.H : Left.H > Right.H;

        if (Left.S != Right.S)
            return ascending ? Left.S < Right.S : Left.S > Right.S;

        return false;
    });
}

/// <summary>
/// Converts a RGB color to HSL.
/// </summary>
ColorHSL RGB2HSL(const D2D1_COLOR_F & rgb)
{
    FLOAT MaxC = (std::max)((std::max)(rgb.r, rgb.g), rgb.b);
    FLOAT MinC = (std::min)((std::min)(rgb.r, rgb.g), rgb.b);
  
    ColorHSL HSL = {};

    if (MaxC != MinC)
    {
        FLOAT d = MaxC - MinC;
   
        if (MaxC == rgb.r)
            HSL.H = (rgb.g - rgb.b) / d + (rgb.g < rgb.b ? 6.f : 0.f);
        else
        if (MaxC == rgb.g)
            HSL.H = (rgb.b - rgb.r) / d + 2.f;
        else
        if (MaxC == rgb.b)
            HSL.H = (rgb.r - rgb.g) / d + 4.f;
    
        HSL.H /= 6.f;
        HSL.S = (HSL.L > 0.5f) ? d / (2.f - MaxC - MinC) : d / (MaxC + MinC);
        HSL.L = (MaxC + MinC) / 2.f;
    }

    return HSL;
}

/// <summary>
/// Converts a HSL color to RGB.
/// </summary>
D2D1_COLOR_F HSL2RGB(const ColorHSL & hsl)
{
    if (hsl.S == 0)
        return D2D1::ColorF(hsl.L, hsl.L, hsl.L); // Achromatic

    FLOAT q = (hsl.L < 0.5f) ? hsl.L * (1.f + hsl.S) : hsl.L + hsl.S - hsl.L * hsl.S;
    FLOAT p = 2.f * hsl.L - q;

    return D2D1::ColorF(EvalHSL(p, q, hsl.H + 1.f / 3.f), EvalHSL(p, q, hsl.H), EvalHSL(p, q, hsl.H - 1.f / 3.f));
}

FLOAT EvalHSL(FLOAT x, FLOAT y, FLOAT z)
{
    if (z < 0) 
        z += 1.f;

    if (z > 1) 
        z -= 1.f;

    if (z < 1.f / 6.f) 
        return x + (y - x) * 6.f * z;

    if (z < 1. / 2.f) 
        return y;

    if (z < 2.f / 3.f)
        return x + (y - x) * (2.f / 3.f - z) * 6.f;
    
    return x;
}

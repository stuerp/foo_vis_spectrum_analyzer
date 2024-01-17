
/** $VER: Gradients.h (2023.12.17) P. Stuer - Built-in gradients. **/

#pragma once

#include "framework.h"

#include <vector>

using namespace std;

// Solid
static const vector<D2D1_GRADIENT_STOP> GradientStopsSolid =
{
    { 1.f, D2D1::ColorF(0x1E90FF, 1.f) },
};

// Custom (default colors)
static const vector<D2D1_GRADIENT_STOP> GradientStopsCustom =
{
    { 0.f / 1.f, D2D1::ColorF(0xbdc3c7, 1.f) },
    { 1.f / 1.f, D2D1::ColorF(0x2c3e50, 1.f) },
};

// Cover Art (default colors)
static const vector<D2D1_GRADIENT_STOP> GradientStopsCoverArt =
{
    { 0.f / 1.f, D2D1::ColorF(D2D1::ColorF::Black) },
    { 1.f / 1.f, D2D1::ColorF(D2D1::ColorF::White) },
};

// Prism / foo_musical_spectrum
static const vector<D2D1_GRADIENT_STOP> GradientStopsPrism1 =
{
    { 0.f / 5.f, D2D1::ColorF(0xFD0000, 1.f) },
    { 1.f / 5.f, D2D1::ColorF(0xFF8000, 1.f) },
    { 2.f / 5.f, D2D1::ColorF(0xFFFF01, 1.f) },
    { 3.f / 5.f, D2D1::ColorF(0x7EFF77, 1.f) },
    { 4.f / 5.f, D2D1::ColorF(0x0193A2, 1.f) },
    { 5.f / 5.f, D2D1::ColorF(0x002161, 1.f) },
};

// Prism 2
static const vector<D2D1_GRADIENT_STOP> GradientStopsPrism2 =
{
    { 0.f / 9.f, D2D1::ColorF(0xAA3355, 1.f) },
    { 1.f / 9.f, D2D1::ColorF(0xCC6666, 1.f) },
    { 2.f / 9.f, D2D1::ColorF(0xEE9944, 1.f) },
    { 3.f / 9.f, D2D1::ColorF(0xEEDD00, 1.f) },
    { 4.f / 9.f, D2D1::ColorF(0x99DD55, 1.f) },
    { 5.f / 9.f, D2D1::ColorF(0x44DD88, 1.f) },
    { 6.f / 9.f, D2D1::ColorF(0x22CCBB, 1.f) },
    { 7.f / 9.f, D2D1::ColorF(0x00BBCC, 1.f) },
    { 8.f / 9.f, D2D1::ColorF(0x0099CC, 1.f) },
    { 9.f / 9.f, D2D1::ColorF(0x3366BB, 1.f) },
};

// Prism 3
static const vector<D2D1_GRADIENT_STOP> GradientStopsPrism3 =
{
    { 0.f / 4.f, D2D1::ColorF(0xFF0000, 1.f) }, // hsl(  0, 100%, 50%)
    { 1.f / 4.f, D2D1::ColorF(0xFFFF00, 1.f) }, // hsl( 60, 100%, 50%)
    { 2.f / 4.f, D2D1::ColorF(0x00FF00, 1.f) }, // hsl(120, 100%, 50%)
    { 3.f / 4.f, D2D1::ColorF(0x00FFFF, 1.f) }, // hsl(180, 100%, 50%)
    { 4.f / 4.f, D2D1::ColorF(0x0000FF, 1.f) }, // hsl(240, 100%, 50%)
};

// foobar2000
static const vector<D2D1_GRADIENT_STOP> GradientStopsFB2K =
{
    { 0.f / 1.f, D2D1::ColorF(0x0066CC, 1.f) }, 
    { 1.f / 1.f, D2D1::ColorF(0x000000, 1.f) },
};

// foobar2000 Dark Mode
static const vector<D2D1_GRADIENT_STOP> GradientStopsFB2KDarkMode =
{
    { 0.f / 1.f, D2D1::ColorF(0x0080FF, 1.f) },
    { 1.f / 1.f, D2D1::ColorF(0xFFFFFF, 1.f) },
};

// Fire (https://www.schemecolor.com/fire-gradient.php)
static const vector<D2D1_GRADIENT_STOP> GradientStopsFire =
{
    { 0.f,   D2D1::ColorF(0xFFF75D, 1.f) },
    { 0.60f, D2D1::ColorF(0xFFC11F, 1.f) },
    { 0.70f, D2D1::ColorF(0xFE650D, 1.f) },
    { 0.80f, D2D1::ColorF(0xF33C04, 1.f) },
    { 0.90f, D2D1::ColorF(0xDA1F05, 1.f) },
    { 1.f,   D2D1::ColorF(0xA10100, 1.f) },
};

static const vector<D2D1_GRADIENT_STOP> GradientStopsRainbow =
{
    {  0.f / 11.f, D2D1::ColorF(0x881177, 1.f) },
    {  1.f / 11.f, D2D1::ColorF(0xAA3355, 1.f) },
    {  2.f / 11.f, D2D1::ColorF(0xCC6666, 1.f) },
    {  3.f / 11.f, D2D1::ColorF(0xEE9944, 1.f) },
    {  4.f / 11.f, D2D1::ColorF(0xEEDD00, 1.f) },
    {  5.f / 11.f, D2D1::ColorF(0x99DD55, 1.f) },
    {  6.f / 11.f, D2D1::ColorF(0x44DD88, 1.f) },
    {  7.f / 11.f, D2D1::ColorF(0x22CCBB, 1.f) },
    {  8.f / 11.f, D2D1::ColorF(0x00BBCC, 1.f) },
    {  9.f / 11.f, D2D1::ColorF(0x0099CC, 1.f) },
    { 10.f / 11.f, D2D1::ColorF(0x3366BB, 1.f) },
    { 11.f / 11.f, D2D1::ColorF(0x663399, 1.f) },
};

/// <summary>
/// Gets a gradient stop vector.
/// </summary>
static const vector<D2D1_GRADIENT_STOP> GetGradientStops(ColorScheme colorScheme)
{
    switch (colorScheme)
    {
        default:

        case ColorScheme::Solid: return GradientStopsSolid; break;
        case ColorScheme::Custom: return GradientStopsCustom; break;
        case ColorScheme::CoverArt: return GradientStopsCoverArt; break;

        case ColorScheme::Prism1: return GradientStopsPrism1; break;
        case ColorScheme::Prism2: return GradientStopsPrism2; break;
        case ColorScheme::Prism3: return GradientStopsPrism3; break;

        case ColorScheme::foobar2000: return GradientStopsFB2K; break;
        case ColorScheme::foobar2000DarkMode: return GradientStopsFB2KDarkMode; break;

        case ColorScheme::Fire: return GradientStopsFire; break;
        case ColorScheme::Rainbow: return GradientStopsRainbow; break;
    };
}

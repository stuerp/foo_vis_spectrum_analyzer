
/** $VER: Gradients.cpp (2026.08.23) P. Stuer - Built-in gradients. **/

#include "pch.h"

#include "Gradients.h"

#include <map>

// Solid
static const gradient_stops_t Solid =
{
    { 1.f, D2D1::ColorF(0x1E90FF, 1.f) },
};

// Custom (default colors)
static const gradient_stops_t Custom =
{
    { 0.f / 1.f, D2D1::ColorF(0xbdc3c7, 1.f) },
    { 1.f / 1.f, D2D1::ColorF(0x2c3e50, 1.f) },
};

// Artwork (default colors)
static const gradient_stops_t Artwork =
{
    { 0.f / 1.f, D2D1::ColorF(D2D1::ColorF::Black) },
    { 1.f / 1.f, D2D1::ColorF(D2D1::ColorF::White) },
};

// Prism / foo_musical_spectrum
static const gradient_stops_t Prism1 =
{
    { 0.f / 5.f, D2D1::ColorF(0xFD0000, 1.f) },
    { 1.f / 5.f, D2D1::ColorF(0xFF8000, 1.f) },
    { 2.f / 5.f, D2D1::ColorF(0xFFFF01, 1.f) },
    { 3.f / 5.f, D2D1::ColorF(0x7EFF77, 1.f) },
    { 4.f / 5.f, D2D1::ColorF(0x0193A2, 1.f) },
    { 5.f / 5.f, D2D1::ColorF(0x002161, 1.f) },
};

// Prism 2
static const gradient_stops_t Prism2 =
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
static const gradient_stops_t Prism3 =
{
    { 0.f / 4.f, D2D1::ColorF(0xFF0000, 1.f) }, // hsl(  0, 100%, 50%)
    { 1.f / 4.f, D2D1::ColorF(0xFFFF00, 1.f) }, // hsl( 60, 100%, 50%)
    { 2.f / 4.f, D2D1::ColorF(0x00FF00, 1.f) }, // hsl(120, 100%, 50%)
    { 3.f / 4.f, D2D1::ColorF(0x00FFFF, 1.f) }, // hsl(180, 100%, 50%)
    { 4.f / 4.f, D2D1::ColorF(0x0000FF, 1.f) }, // hsl(240, 100%, 50%)
};

// foobar2000
static const gradient_stops_t foobar2000 =
{
    { 0.f / 1.f, D2D1::ColorF(0x0066CC, 1.f) }, 
    { 1.f / 1.f, D2D1::ColorF(0x000000, 1.f) },
};

// foobar2000 Dark Mode
static const gradient_stops_t foobar2000DarkMode =
{
    { 0.f / 1.f, D2D1::ColorF(0x0080FF, 1.f) },
    { 1.f / 1.f, D2D1::ColorF(0xFFFFFF, 1.f) },
};

// Fire (https://www.schemecolor.com/fire-gradient.php)
static const gradient_stops_t Fire =
{
    { 0.f,  D2D1::ColorF(0xFFF75D, 1.f) },
    { 0.6f, D2D1::ColorF(0xFFC11F, 1.f) },
    { 0.7f, D2D1::ColorF(0xFE650D, 1.f) },
    { 0.8f, D2D1::ColorF(0xF33C04, 1.f) },
    { 0.9f, D2D1::ColorF(0xDA1F05, 1.f) },
    { 1.f,  D2D1::ColorF(0xA10100, 1.f) },
};

static const gradient_stops_t Rainbow =
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

// SoX (https://sourceforge.net/p/sox/code/ci/master/tree/)
static const gradient_stops_t SoX =
{
/*
    { 0.00f, D2D1::ColorF(1.00f, 1.00f, 1.00f, 1.f) },
    { 0.09f, D2D1::ColorF(1.00f, 1.00f, 0.59f, 1.f) },
    { 0.22f, D2D1::ColorF(1.00f, 0.79f, 0.00f, 1.f) },
    { 0.27f, D2D1::ColorF(1.00f, 0.61f, 0.00f, 1.f) },
    { 0.40f, D2D1::ColorF(0.94f, 0.00f, 0.00f, 1.f) },
    { 0.87f, D2D1::ColorF(0.00f, 0.00f, 0.31f, 1.f) },
    { 1.00f, D2D1::ColorF(0.00f, 0.00f, 0.00f, 1.f) },
*/
    { 0.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f) }, // White
    { 0.2f, D2D1::ColorF(1.0f, 1.0f, 0.0f) }, // Yellow
    { 0.4f, D2D1::ColorF(1.0f, 0.0f, 0.0f) }, // Red
    { 0.6f, D2D1::ColorF(1.0f, 0.0f, 1.0f) }, // Magenta
    { 0.8f, D2D1::ColorF(0.0f, 0.0f, 1.0f) }, // Blue
    { 1.0f, D2D1::ColorF(0.0f, 0.0f, 0.0f) }, // Black
};

// Turbo (https://research.google/blog/turbo-an-improved-rainbow-colormap-for-visualization/), Classic spectrum analyzer appearance
static const gradient_stops_t Turbo =
{
    { 0.000f, D2D1::ColorF(0.480f, 0.016f, 0.011f) },
    { 0.032f, D2D1::ColorF(0.572f, 0.045f, 0.005f) },
    { 0.065f, D2D1::ColorF(0.664f, 0.084f, 0.004f) },
    { 0.097f, D2D1::ColorF(0.747f, 0.131f, 0.009f) },
    { 0.129f, D2D1::ColorF(0.816f, 0.185f, 0.018f) },
    { 0.161f, D2D1::ColorF(0.876f, 0.245f, 0.033f) },
    { 0.194f, D2D1::ColorF(0.926f, 0.315f, 0.055f) },
    { 0.226f, D2D1::ColorF(0.962f, 0.389f, 0.084f) },
    { 0.258f, D2D1::ColorF(0.985f, 0.469f, 0.118f) },
    { 0.290f, D2D1::ColorF(0.995f, 0.552f, 0.154f) },
    { 0.323f, D2D1::ColorF(0.997f, 0.631f, 0.187f) },
    { 0.355f, D2D1::ColorF(0.989f, 0.703f, 0.214f) },
    { 0.387f, D2D1::ColorF(0.960f, 0.772f, 0.228f) },
    { 0.419f, D2D1::ColorF(0.913f, 0.836f, 0.223f) },
    { 0.452f, D2D1::ColorF(0.850f, 0.893f, 0.211f) },
    { 0.484f, D2D1::ColorF(0.776f, 0.941f, 0.203f) },
    { 0.516f, D2D1::ColorF(0.695f, 0.976f, 0.213f) },
    { 0.548f, D2D1::ColorF(0.611f, 0.995f, 0.253f) },
    { 0.581f, D2D1::ColorF(0.519f, 0.999f, 0.316f) },
    { 0.613f, D2D1::ColorF(0.428f, 0.994f, 0.386f) },
    { 0.645f, D2D1::ColorF(0.336f, 0.983f, 0.463f) },
    { 0.677f, D2D1::ColorF(0.248f, 0.964f, 0.543f) },
    { 0.710f, D2D1::ColorF(0.174f, 0.941f, 0.619f) },
    { 0.742f, D2D1::ColorF(0.120f, 0.912f, 0.687f) },
    { 0.774f, D2D1::ColorF(0.094f, 0.878f, 0.743f) },
    { 0.806f, D2D1::ColorF(0.101f, 0.830f, 0.814f) },
    { 0.839f, D2D1::ColorF(0.139f, 0.763f, 0.896f) },
    { 0.871f, D2D1::ColorF(0.201f, 0.678f, 0.968f) },
    { 0.903f, D2D1::ColorF(0.259f, 0.580f, 0.999f) },
    { 0.935f, D2D1::ColorF(0.276f, 0.431f, 0.903f) },
    { 0.968f, D2D1::ColorF(0.244f, 0.231f, 0.537f) },
    { 1.000f, D2D1::ColorF(0.190f, 0.072f, 0.232f) }
};

// Viridis (https://matplotlib.org/stable/users/explain/colors/colormaps.html), Best quantitative interpretation
static const gradient_stops_t Viridis =
{
    { 0.000f, D2D1::ColorF(0.993f, 0.906f, 0.144f) },
    { 0.032f, D2D1::ColorF(0.983f, 0.904f, 0.225f) },
    { 0.065f, D2D1::ColorF(0.945f, 0.835f, 0.176f) },
    { 0.097f, D2D1::ColorF(0.867f, 0.806f, 0.125f) },
    { 0.129f, D2D1::ColorF(0.773f, 0.792f, 0.109f) },
    { 0.161f, D2D1::ColorF(0.678f, 0.779f, 0.102f) },
    { 0.194f, D2D1::ColorF(0.586f, 0.764f, 0.116f) },
    { 0.226f, D2D1::ColorF(0.497f, 0.749f, 0.150f) },
    { 0.258f, D2D1::ColorF(0.414f, 0.732f, 0.192f) },
    { 0.290f, D2D1::ColorF(0.336f, 0.713f, 0.239f) },
    { 0.323f, D2D1::ColorF(0.267f, 0.691f, 0.285f) },
    { 0.355f, D2D1::ColorF(0.207f, 0.668f, 0.329f) },
    { 0.387f, D2D1::ColorF(0.163f, 0.643f, 0.366f) },
    { 0.419f, D2D1::ColorF(0.134f, 0.617f, 0.403f) },
    { 0.452f, D2D1::ColorF(0.125f, 0.589f, 0.438f) },
    { 0.484f, D2D1::ColorF(0.129f, 0.559f, 0.462f) },
    { 0.516f, D2D1::ColorF(0.136f, 0.529f, 0.485f) },
    { 0.548f, D2D1::ColorF(0.144f, 0.498f, 0.505f) },
    { 0.581f, D2D1::ColorF(0.153f, 0.466f, 0.520f) },
    { 0.613f, D2D1::ColorF(0.163f, 0.434f, 0.534f) },
    { 0.645f, D2D1::ColorF(0.174f, 0.401f, 0.545f) },
    { 0.677f, D2D1::ColorF(0.188f, 0.368f, 0.551f) },
    { 0.710f, D2D1::ColorF(0.203f, 0.334f, 0.557f) },
    { 0.742f, D2D1::ColorF(0.220f, 0.301f, 0.560f) },
    { 0.774f, D2D1::ColorF(0.237f, 0.266f, 0.553f) },
    { 0.806f, D2D1::ColorF(0.253f, 0.231f, 0.537f) },
    { 0.839f, D2D1::ColorF(0.268f, 0.196f, 0.516f) },
    { 0.871f, D2D1::ColorF(0.278f, 0.162f, 0.483f) },
    { 0.903f, D2D1::ColorF(0.283f, 0.126f, 0.445f) },
    { 0.935f, D2D1::ColorF(0.283f, 0.089f, 0.412f) },
    { 0.968f, D2D1::ColorF(0.278f, 0.047f, 0.376f) },
    { 1.000f, D2D1::ColorF(0.267f, 0.005f, 0.329f) }
};

// Plasma (https://matplotlib.org/stable/users/explain/colors/colormaps.html), Very vibrant, good contrast
static const gradient_stops_t Plasma =
{
    { 0.000f, D2D1::ColorF(0.940f, 0.975f, 0.131f) },
    { 0.032f, D2D1::ColorF(0.997f, 0.967f, 0.420f) },
    { 0.065f, D2D1::ColorF(0.996f, 0.946f, 0.310f) },
    { 0.097f, D2D1::ColorF(0.992f, 0.923f, 0.225f) },
    { 0.129f, D2D1::ColorF(0.985f, 0.893f, 0.156f) },
    { 0.161f, D2D1::ColorF(0.976f, 0.856f, 0.104f) },
    { 0.194f, D2D1::ColorF(0.966f, 0.814f, 0.066f) },
    { 0.226f, D2D1::ColorF(0.955f, 0.765f, 0.050f) },
    { 0.258f, D2D1::ColorF(0.945f, 0.712f, 0.047f) },
    { 0.290f, D2D1::ColorF(0.935f, 0.658f, 0.053f) },
    { 0.323f, D2D1::ColorF(0.924f, 0.603f, 0.067f) },
    { 0.355f, D2D1::ColorF(0.912f, 0.545f, 0.096f) },
    { 0.387f, D2D1::ColorF(0.899f, 0.488f, 0.146f) },
    { 0.419f, D2D1::ColorF(0.883f, 0.434f, 0.209f) },
    { 0.452f, D2D1::ColorF(0.865f, 0.381f, 0.274f) },
    { 0.484f, D2D1::ColorF(0.843f, 0.332f, 0.338f) },
    { 0.516f, D2D1::ColorF(0.819f, 0.284f, 0.401f) },
    { 0.548f, D2D1::ColorF(0.790f, 0.239f, 0.458f) },
    { 0.581f, D2D1::ColorF(0.757f, 0.197f, 0.511f) },
    { 0.613f, D2D1::ColorF(0.720f, 0.157f, 0.556f) },
    { 0.645f, D2D1::ColorF(0.680f, 0.118f, 0.592f) },
    { 0.677f, D2D1::ColorF(0.635f, 0.084f, 0.619f) },
    { 0.710f, D2D1::ColorF(0.587f, 0.054f, 0.639f) },
    { 0.742f, D2D1::ColorF(0.536f, 0.029f, 0.652f) },
    { 0.774f, D2D1::ColorF(0.483f, 0.009f, 0.660f) },
    { 0.806f, D2D1::ColorF(0.429f, 0.001f, 0.659f) },
    { 0.839f, D2D1::ColorF(0.372f, 0.005f, 0.652f) },
    { 0.871f, D2D1::ColorF(0.318f, 0.009f, 0.637f) },
    { 0.903f, D2D1::ColorF(0.261f, 0.013f, 0.617f) },
    { 0.935f, D2D1::ColorF(0.202f, 0.018f, 0.593f) },
    { 0.968f, D2D1::ColorF(0.132f, 0.022f, 0.563f) },
    { 1.000f, D2D1::ColorF(0.050f, 0.030f, 0.528f) }
};

// Inferno (https://matplotlib.org/stable/users/explain/colors/colormaps.html), Best weak-signal visibility
static const gradient_stops_t Inferno =
{
    { 0.000f, D2D1::ColorF(0.988f, 0.998f, 0.645f) },
    { 0.032f, D2D1::ColorF(0.993f, 0.988f, 0.470f) },
    { 0.065f, D2D1::ColorF(0.990f, 0.958f, 0.250f) },
    { 0.097f, D2D1::ColorF(0.989f, 0.910f, 0.215f) },
    { 0.129f, D2D1::ColorF(0.985f, 0.844f, 0.225f) },
    { 0.161f, D2D1::ColorF(0.977f, 0.769f, 0.255f) },
    { 0.194f, D2D1::ColorF(0.965f, 0.691f, 0.292f) },
    { 0.226f, D2D1::ColorF(0.948f, 0.615f, 0.329f) },
    { 0.258f, D2D1::ColorF(0.927f, 0.543f, 0.366f) },
    { 0.290f, D2D1::ColorF(0.901f, 0.475f, 0.403f) },
    { 0.323f, D2D1::ColorF(0.869f, 0.414f, 0.438f) },
    { 0.355f, D2D1::ColorF(0.833f, 0.360f, 0.468f) },
    { 0.387f, D2D1::ColorF(0.792f, 0.313f, 0.494f) },
    { 0.419f, D2D1::ColorF(0.748f, 0.273f, 0.515f) },
    { 0.452f, D2D1::ColorF(0.700f, 0.240f, 0.529f) },
    { 0.484f, D2D1::ColorF(0.651f, 0.214f, 0.537f) },
    { 0.516f, D2D1::ColorF(0.600f, 0.193f, 0.539f) },
    { 0.548f, D2D1::ColorF(0.548f, 0.177f, 0.535f) },
    { 0.581f, D2D1::ColorF(0.497f, 0.165f, 0.525f) },
    { 0.613f, D2D1::ColorF(0.447f, 0.154f, 0.510f) },
    { 0.645f, D2D1::ColorF(0.398f, 0.145f, 0.491f) },
    { 0.677f, D2D1::ColorF(0.352f, 0.135f, 0.466f) },
    { 0.710f, D2D1::ColorF(0.307f, 0.125f, 0.438f) },
    { 0.742f, D2D1::ColorF(0.263f, 0.115f, 0.406f) },
    { 0.774f, D2D1::ColorF(0.221f, 0.104f, 0.370f) },
    { 0.806f, D2D1::ColorF(0.181f, 0.091f, 0.330f) },
    { 0.839f, D2D1::ColorF(0.143f, 0.078f, 0.285f) },
    { 0.871f, D2D1::ColorF(0.107f, 0.063f, 0.237f) },
    { 0.903f, D2D1::ColorF(0.075f, 0.048f, 0.185f) },
    { 0.935f, D2D1::ColorF(0.048f, 0.030f, 0.129f) },
    { 0.968f, D2D1::ColorF(0.024f, 0.012f, 0.071f) },
    { 1.000f, D2D1::ColorF(0.001f, 0.000f, 0.014f) }
};

// Magma (https://matplotlib.org/stable/users/explain/colors/colormaps.html), Similar to Inferno but slightly softer
static const gradient_stops_t Magma =
{
    { 0.000f, D2D1::ColorF(0.987f, 0.991f, 0.750f) },
    { 0.032f, D2D1::ColorF(0.997f, 0.991f, 0.549f) },
    { 0.065f, D2D1::ColorF(0.994f, 0.972f, 0.332f) },
    { 0.097f, D2D1::ColorF(0.980f, 0.930f, 0.255f) },
    { 0.129f, D2D1::ColorF(0.966f, 0.872f, 0.247f) },
    { 0.161f, D2D1::ColorF(0.953f, 0.804f, 0.264f) },
    { 0.194f, D2D1::ColorF(0.938f, 0.731f, 0.294f) },
    { 0.226f, D2D1::ColorF(0.920f, 0.657f, 0.332f) },
    { 0.258f, D2D1::ColorF(0.899f, 0.583f, 0.374f) },
    { 0.290f, D2D1::ColorF(0.874f, 0.512f, 0.418f) },
    { 0.323f, D2D1::ColorF(0.845f, 0.444f, 0.461f) },
    { 0.355f, D2D1::ColorF(0.812f, 0.380f, 0.503f) },
    { 0.387f, D2D1::ColorF(0.775f, 0.322f, 0.541f) },
    { 0.419f, D2D1::ColorF(0.734f, 0.270f, 0.573f) },
    { 0.452f, D2D1::ColorF(0.689f, 0.226f, 0.597f) },
    { 0.484f, D2D1::ColorF(0.641f, 0.190f, 0.612f) },
    { 0.516f, D2D1::ColorF(0.591f, 0.163f, 0.618f) },
    { 0.548f, D2D1::ColorF(0.539f, 0.145f, 0.615f) },
    { 0.581f, D2D1::ColorF(0.487f, 0.134f, 0.606f) },
    { 0.613f, D2D1::ColorF(0.437f, 0.126f, 0.592f) },
    { 0.645f, D2D1::ColorF(0.388f, 0.119f, 0.573f) },
    { 0.677f, D2D1::ColorF(0.340f, 0.112f, 0.547f) },
    { 0.710f, D2D1::ColorF(0.294f, 0.104f, 0.516f) },
    { 0.742f, D2D1::ColorF(0.250f, 0.095f, 0.480f) },
    { 0.774f, D2D1::ColorF(0.207f, 0.086f, 0.439f) },
    { 0.806f, D2D1::ColorF(0.167f, 0.075f, 0.393f) },
    { 0.839f, D2D1::ColorF(0.130f, 0.064f, 0.342f) },
    { 0.871f, D2D1::ColorF(0.097f, 0.052f, 0.287f) },
    { 0.903f, D2D1::ColorF(0.068f, 0.040f, 0.229f) },
    { 0.935f, D2D1::ColorF(0.043f, 0.029f, 0.168f) },
    { 0.968f, D2D1::ColorF(0.021f, 0.018f, 0.105f) },
    { 1.000f, D2D1::ColorF(0.001f, 0.000f, 0.014f) }
};

// Cividis (https://matplotlib.org/stable/users/explain/colors/colormaps.html), Best color-blind accessibility
static const gradient_stops_t Cividis =
{
    { 0.000f, D2D1::ColorF(0.996f, 0.909f, 0.218f) },
    { 0.032f, D2D1::ColorF(0.956f, 0.827f, 0.279f) },
    { 0.065f, D2D1::ColorF(0.920f, 0.794f, 0.337f) },
    { 0.097f, D2D1::ColorF(0.884f, 0.763f, 0.385f) },
    { 0.129f, D2D1::ColorF(0.848f, 0.733f, 0.425f) },
    { 0.161f, D2D1::ColorF(0.812f, 0.705f, 0.458f) },
    { 0.194f, D2D1::ColorF(0.776f, 0.678f, 0.486f) },
    { 0.226f, D2D1::ColorF(0.741f, 0.652f, 0.510f) },
    { 0.258f, D2D1::ColorF(0.706f, 0.627f, 0.531f) },
    { 0.290f, D2D1::ColorF(0.672f, 0.603f, 0.548f) },
    { 0.323f, D2D1::ColorF(0.638f, 0.579f, 0.563f) },
    { 0.355f, D2D1::ColorF(0.605f, 0.556f, 0.574f) },
    { 0.387f, D2D1::ColorF(0.572f, 0.534f, 0.584f) },
    { 0.419f, D2D1::ColorF(0.540f, 0.512f, 0.591f) },
    { 0.452f, D2D1::ColorF(0.508f, 0.490f, 0.597f) },
    { 0.484f, D2D1::ColorF(0.476f, 0.469f, 0.601f) },
    { 0.516f, D2D1::ColorF(0.445f, 0.448f, 0.603f) },
    { 0.548f, D2D1::ColorF(0.414f, 0.427f, 0.603f) },
    { 0.581f, D2D1::ColorF(0.383f, 0.406f, 0.602f) },
    { 0.613f, D2D1::ColorF(0.352f, 0.386f, 0.599f) },
    { 0.645f, D2D1::ColorF(0.321f, 0.365f, 0.595f) },
    { 0.677f, D2D1::ColorF(0.289f, 0.344f, 0.588f) },
    { 0.710f, D2D1::ColorF(0.256f, 0.323f, 0.579f) },
    { 0.742f, D2D1::ColorF(0.221f, 0.303f, 0.567f) },
    { 0.774f, D2D1::ColorF(0.183f, 0.282f, 0.551f) },
    { 0.806f, D2D1::ColorF(0.141f, 0.261f, 0.530f) },
    { 0.839f, D2D1::ColorF(0.094f, 0.240f, 0.504f) },
    { 0.871f, D2D1::ColorF(0.039f, 0.219f, 0.468f) },
    { 0.903f, D2D1::ColorF(0.000f, 0.198f, 0.428f) },
    { 0.935f, D2D1::ColorF(0.000f, 0.177f, 0.387f) },
    { 0.968f, D2D1::ColorF(0.000f, 0.156f, 0.345f) },
    { 1.000f, D2D1::ColorF(0.000f, 0.135f, 0.304f) }
};

static const std::map<ColorScheme, const gradient_stops_t *> ColorMaps
{
    { ColorScheme::Solid,               &Solid },
    { ColorScheme::Custom,              &Custom },
    { ColorScheme::Artwork,             &Artwork },

    { ColorScheme::Prism1,              &Prism1 },
    { ColorScheme::Prism2,              &Prism2 },
    { ColorScheme::Prism3,              &Prism3 },

    { ColorScheme::foobar2000,          &foobar2000 },
    { ColorScheme::foobar2000DarkMode,  &foobar2000DarkMode },

    { ColorScheme::Fire,                &Fire },
    { ColorScheme::Rainbow,             &Rainbow },

    { ColorScheme::SoX,                 &SoX },

    // v0.12.0.0
    { ColorScheme::Turbo,               &Turbo },

    // v0.12.0.0, Matplotlib
    { ColorScheme::Viridis,             &Viridis },
    { ColorScheme::Plasma,              &Plasma },
    { ColorScheme::Inferno,             &Inferno },
    { ColorScheme::Magma,               &Magma },
    { ColorScheme::Cividis,             &Cividis },
};

/// <summary>
/// Gets a gradient stop vector.
/// </summary>
const gradient_stops_t & GetBuiltInGradientStops(ColorScheme colorScheme) noexcept
{
    if (auto Iter = ColorMaps.find(colorScheme); Iter != ColorMaps.end())
        return *Iter->second;

    return Solid;
}


/** $VER: Support.h (2024.01.28) P. Stuer **/

#pragma once

#include "framework.h"

#include <math.h>

HRESULT GetDPI(HWND hWnd, UINT & dpi);

/// <summary>
/// Returns the minimum value of the specified values.
/// </summary>
template <class T>
inline static T Min(T a, T b)
{
    return (a < b) ? a : b;
}

/// <summary>
/// Returns the maximum value of the specified values.
/// </summary>
template <class T>
inline static T Max(T a, T b)
{
    return (a > b) ? a : b;
}

/// <summary>
/// Returns the input value clamped between min and max.
/// </summary>
template <class T>
inline static T Clamp(T value, T minValue, T maxValue)
{
    return Min(Max(value, minValue), maxValue);
}

/// <summary>
/// Returns true of the input value is in the interval between min and max.
/// </summary>
template <class T>
inline static T InRange(T value, T minValue, T maxValue)
{
    return (minValue <= value) && (value <= maxValue);
}

/// <summary>
/// Converts magnitude to decibel (dB).
/// </summary>
inline static double ToDecibel(double magnitude)
{
    return 20.0 * ::log10(magnitude);
}

/// <summary>
/// Converts decibel (dB) to magnitude.
/// </summary>
inline static double ToMagnitude(double dB)
{
    return ::pow(10.0, dB / 20.0);
}

/// <summary>
/// Converts points to DIPs (Device Independent Pixels).
/// </summary>
inline static FLOAT ToDIPs(FLOAT points)
{

    return (points / 72.0f) * (FLOAT) USER_DEFAULT_SCREEN_DPI; // FIXME: Should 96.0 change on high DPI screens?
}

/// <summary>
/// Wraps an index.
/// </summary>
template<class T>
inline static T Wrap(T index, T length)
{
    return (length + (index % length)) % length;
}

/// <summary>
/// Maps a value from one range (srcMin, srcMax) to another (dstMin, dstMax).
/// </summary>
template<class T, class U>
inline static U Map(T value, T srcMin, T srcMax, U dstMin, U dstMax)
{
    return dstMin + (U) (((double) (value - srcMin) * (dstMax - dstMin)) / (double) (srcMax - srcMin));
}

/// <summary>
/// 
/// </summary>
inline static double LogSpace(double minFreq, double maxFreq, double bandIndex, size_t maxBands, double skewFactor)
{
    const double CenterFreq = minFreq * ::pow((maxFreq / minFreq), (bandIndex / (double) maxBands));

    return CenterFreq * (1 - skewFactor) + (minFreq + ((maxFreq - minFreq) * bandIndex * (1. / (double) maxBands))) * skewFactor;
}

/// <summary>
/// Converts a COLORREF to a D2D1_COLOR_F.
/// </summary>
inline D2D1_COLOR_F ToD2D1_COLOR_F(COLORREF color)
{
    return  D2D1::ColorF(GetRValue(color) / 255.f, GetGValue(color) / 255.f, GetBValue(color) / 255.f);
}

/// <summary>
/// Converts a D2D1_COLOR_F to a COLORREF.
/// </summary>
inline COLORREF ToCOLORREF(const D2D1_COLOR_F & color)
{
    return RGB((BYTE)(color.r * 255.f), (BYTE)(color.g * 255.f), (BYTE)(color.b * 255.f));
}

/// <summary>
/// Gets the handle of the module that contains the executing code.
/// </summary>
inline HMODULE GetCurrentModule() noexcept
{
    HMODULE hModule = NULL;

    ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) GetCurrentModule, &hModule);

    return hModule;
}

/*
static bool SelectColor(HWND hWnd, D2D1_COLOR_F & color)
{
    static COLORREF CustomColors[16] =
    {
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
        RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF), RGB(0xFF,0xFF,0xFF),
    };

    CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };

    cc.hwndOwner = hWnd;

    cc.lpCustColors = (LPDWORD) CustomColors;
    cc.rgbResult = ToCOLORREF(color);
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;

    if (!::ChooseColorW(&cc))
        return false;

    color = ToD2D1_COLOR_F(cc.rgbResult);

    return true;
}
*/

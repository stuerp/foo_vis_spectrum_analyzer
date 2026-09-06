
/** $VER: Support.h (2026.09.06) P. Stuer **/

#pragma once

#include <sdkddkver.h>
#include <Windows.h>

#include <cmath>

HRESULT InitializeDpiAwareness() noexcept;
HRESULT GetDPI(_In_ HWND hWnd, _Out_ UINT & dpi) noexcept;
HRESULT EvaluateTitleFormatScript(_In_ const std::wstring & script, _Out_ pfc::string & result) noexcept;

/// <summary>
/// Converts magnitude to decibel (dB).
/// </summary>
inline static double ToDecibel(const double magnitude) noexcept
{
    return 20. * std::log10(magnitude);
}

/// <summary>
/// Converts decibel (dB) to magnitude.
/// </summary>
inline static double ToMagnitude(const double dB) noexcept
{
    return std::pow(10., dB / 20.);
}

/// <summary>
/// Converts points to DIPs (Device Independent Pixels).
/// </summary>
inline static FLOAT ToDIPs(const FLOAT points) noexcept
{
    return (points / 72.f) * (FLOAT) USER_DEFAULT_SCREEN_DPI; // FIXME: Should 96.0 change on high DPI screens?
}

/// <summary>
/// Converts the specified value from degrees to radians.
/// </summary>
inline double Degrees2Radians(double degrees) noexcept
{
    return (degrees * 2. * std::numbers::pi) / 360.;
}

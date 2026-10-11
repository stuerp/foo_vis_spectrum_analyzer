
/** $VER: Support.h (2026.10.02) P. Stuer **/

#pragma once

#include <sdkddkver.h>
#include <Windows.h>

#include <cmath>

HRESULT InitializeDpiAwareness() noexcept;
HRESULT GetDPI(HWND hWnd, UINT & dpi) noexcept;
HRESULT EvaluateTitleFormatScript(const std::wstring & script, pfc::string & result) noexcept;

/// <summary>
/// Converts magnitude to decibel (dB).
/// </summary>
[[nodiscard]] inline double ToDecibel(const double magnitude) noexcept
{
    return 20. * std::log10(magnitude);
}

/// <summary>
/// Converts decibel (dB) to magnitude.
/// </summary>
[[nodiscard]] inline double ToMagnitude(const double dB) noexcept
{
    return std::pow(10., dB / 20.);
}

/// <summary>
/// Converts typographic points to DIPs (Device Independent Pixels).
/// </summary>
/// <remarks>
/// One point is 1/72 of an inch and one DIP is 1/96 of a logical inch.
/// </remarks>
[[nodiscard]] inline constexpr FLOAT PointsToDIPs(const FLOAT points) noexcept
{
    constexpr FLOAT DIPSPerInch = 96.0f;
    constexpr FLOAT PointsPerInch = 72.0f;

    return points * DIPSPerInch / PointsPerInch;
}

/// <summary>
/// Converts the specified value from degrees to radians.
/// </summary>
[[nodiscard]] inline constexpr double DegreesToRadians(double degrees) noexcept
{
    return degrees * std::numbers::pi / 180.0;
}

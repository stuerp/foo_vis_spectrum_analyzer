
/** $VER: Support.h (2026.09.05) P. Stuer **/

#pragma once

#include <sdkddkver.h>
#include <Windows.h>

#include <math.h>

#include "Constants.h"

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
/// Calculates the scale factor from the specified frequency.
/// </summary>
inline double ScaleFrequency(const double f, const ScalingFunction function, const double skewFactor) noexcept
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return f;

        case ScalingFunction::Logarithmic:
            return std::log2(f);

        case ScalingFunction::ShiftedLogarithmic:
            return std::log2(std::pow(10, skewFactor * 4.) + f);

        case ScalingFunction::Mel:
            return std::log2(1. + f / 700.);

        case ScalingFunction::Bark: // "Critical bands"
            return (26.81 * f) / (1960. + f) - 0.53;

        case ScalingFunction::AdjustableBark:
            return (26.81 * f) / (std::pow(10, skewFactor * 4.) + f);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return std::log2(1. + 0.00437 * f);

        case ScalingFunction::Cams:
            return std::log2((f / 1000. + 0.312) / (f / 1000. + 14.675));

        case ScalingFunction::HyperbolicSine:
            return std::asinh(f / std::pow(10, skewFactor * 4));

        case ScalingFunction::NthRoot:
            return std::pow(f, (1. / (11. - skewFactor * 10.)));

        case ScalingFunction::NegativeExponential:
            return -std::exp2(-f / std::exp2(7. + skewFactor * 8.));

        case ScalingFunction::Period:
            return 1. / f;
    }
}

/// <summary>
/// Calculates the frequency from the specified scale factor.
/// </summary>
inline double DescaleFrequency(const double x, const ScalingFunction function, const double skewFactor) noexcept
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return std::exp2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return std::exp2(x) - std::pow(10., skewFactor * 4.);

        case ScalingFunction::Mel:
            return 700. * (std::exp2(x) - 1.);

        case ScalingFunction::Bark: // "Critical bands"
            return 1960. / (26.81 / (x + 0.53) - 1.);

        case ScalingFunction::AdjustableBark:
            return std::pow(10., (skewFactor * 4.)) / (26.81 / x - 1.);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return (1. / 0.00437) * (std::exp2(x) - 1.);

        case ScalingFunction::Cams:
            return (14.675 * std::exp2(x) - 0.312) / (1. - std::exp2(x)) * 1000.;

        case ScalingFunction::HyperbolicSine:
            return std::sinh(x) * std::pow(10., skewFactor * 4.);

        case ScalingFunction::NthRoot:
            return std::pow(x, ((11. - skewFactor * 10.)));

        case ScalingFunction::NegativeExponential:
            return -std::log2(-x) * std::exp2(7. + skewFactor * 8.);

        case ScalingFunction::Period:
            return 1. / x;
    }
}

/// <summary>
/// 
/// </summary>
inline double LogSpace(double minFreq, double maxFreq, double bandIndex, size_t maxBands, double skewFactor) noexcept
{
    const double CenterFreq = minFreq * std::pow((maxFreq / minFreq), (bandIndex / (double) maxBands));

    return CenterFreq * (1. - skewFactor) + (minFreq + ((maxFreq - minFreq) * bandIndex * (1. / (double) maxBands))) * skewFactor;
}

/// <summary>
/// Converts the specified value from degrees to radians.
/// </summary>
inline double Degrees2Radians(double degrees) noexcept
{
    return (degrees * 2. * std::numbers::pi) / 360.;
}

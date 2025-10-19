
/** $VER: Support.h (2025.10.20) P. Stuer **/

#pragma once

#include <sdkddkver.h>
#include <Windows.h>

#include <math.h>

#include "Constants.h"

HRESULT InitializeDpiAwareness() noexcept;
HRESULT GetDPI(_In_ HWND hWnd, _Out_ UINT & dpi) noexcept;

/// <summary>
/// Converts magnitude to decibel (dB).
/// </summary>
inline static double ToDecibel(const double magnitude) noexcept
{
    return 20.0 * ::log10(magnitude);
}

/// <summary>
/// Converts decibel (dB) to magnitude.
/// </summary>
inline static double ToMagnitude(const double dB) noexcept
{
    return ::pow(10.0, dB / 20.0);
}

/// <summary>
/// Converts points to DIPs (Device Independent Pixels).
/// </summary>
inline static FLOAT ToDIPs(const FLOAT points) noexcept
{
    return (points / 72.0f) * (FLOAT) USER_DEFAULT_SCREEN_DPI; // FIXME: Should 96.0 change on high DPI screens?
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
            return ::log2(f);

        case ScalingFunction::ShiftedLogarithmic:
            return ::log2(::pow(10, skewFactor * 4.0) + f);

        case ScalingFunction::Mel:
            return ::log2(1.0 + f / 700.0);

        case ScalingFunction::Bark: // "Critical bands"
            return (26.81 * f) / (1960.0 + f) - 0.53;

        case ScalingFunction::AdjustableBark:
            return (26.81 * f) / (::pow(10, skewFactor * 4.0) + f);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return ::log2(1.0 + 0.00437 * f);

        case ScalingFunction::Cams:
            return ::log2((f / 1000.0 + 0.312) / (f / 1000.0 + 14.675));

        case ScalingFunction::HyperbolicSine:
            return ::asinh(f / ::pow(10, skewFactor * 4));

        case ScalingFunction::NthRoot:
            return ::pow(f, (1.0 / (11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::exp2(-f / ::exp2(7 + skewFactor * 8));

        case ScalingFunction::Period:
            return 1.0 / f;
    }
}

/// <summary>
/// Calculates the frequency from the specified scale factor.
/// </summary>
inline double DeScaleF(const double x, const ScalingFunction function, const double skewFactor) noexcept
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::exp2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::exp2(x) - ::pow(10.0, skewFactor * 4.0);

        case ScalingFunction::Mel:
            return 700.0 * (::exp2(x) - 1.0);

        case ScalingFunction::Bark: // "Critical bands"
            return 1960.0 / (26.81 / (x + 0.53) - 1.0);

        case ScalingFunction::AdjustableBark:
            return ::pow(10.0, (skewFactor * 4.0)) / (26.81 / x - 1.0);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return (1 / 0.00437) * (::exp2(x) - 1);

        case ScalingFunction::Cams:
            return (14.675 * ::exp2(x) - 0.312) / (1.0 - ::exp2(x)) * 1000.0;

        case ScalingFunction::HyperbolicSine:
            return ::sinh(x) * ::pow(10.0, skewFactor * 4);

        case ScalingFunction::NthRoot:
            return ::pow(x, ((11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::log2(-x) * ::exp2(7.0 + skewFactor * 8.0);

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// 
/// </summary>
inline double LogSpace(double minFreq, double maxFreq, double bandIndex, size_t maxBands, double skewFactor) noexcept
{
    const double CenterFreq = minFreq * ::pow((maxFreq / minFreq), (bandIndex / (double) maxBands));

    return CenterFreq * (1 - skewFactor) + (minFreq + ((maxFreq - minFreq) * bandIndex * (1. / (double) maxBands))) * skewFactor;
}

/// <summary>
/// Converts the specified value from degrees to radians.
/// </summary>
inline double Degrees2Radians(double degrees) noexcept
{
    return (degrees * 2 * M_PI) / 360.;
}


/** $VER: FrequencyScaler.h (2026.09.06) P. Stuer **/

#pragma once

#include <cmath>

#include "Constants.h"

/// <summary>
/// Calculates the scale factor from the specified frequency.
/// </summary>
[[nodiscard]] inline double ScaleFrequency(const double f, const ScalingFunction function, const double skewFactor) noexcept
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
[[nodiscard]] inline double DescaleFrequency(const double x, const ScalingFunction function, const double skewFactor) noexcept
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

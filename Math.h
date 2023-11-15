
/** $VER: Math.h (2023.11.15) P. Stuer - Math support routines **/

#pragma once

#define _USE_MATH_DEFINES

#include <math.h>

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
inline static T InInterval(T value, T minValue, T maxValue)
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
/// Converts points to DIPs (Device Independet Pixels).
/// </summary>
inline static FLOAT ToDIPs(FLOAT points)
{
    return (points / 72.0f) * 96.0f; // FIXME: Should 96.0 change on high DPI screens?
}

/// <summary>
/// Maps a value from one range (srcMin, srcMax) to another (dstMin, dstMax).
/// </summary>
inline static double Map(double value, double srcMin, double srcMax, double dstMin, double dstMax)
{
    return dstMin + ((value - srcMin) * (dstMax - dstMin)) / (srcMax - srcMin);
}

/// <summary>
/// 
/// </summary>
inline static double LogSpace(uint32_t minFreq, uint32_t maxFreq, double bandIndex, size_t maxBands, double skewFactor)
{
    const double CenterFreq = minFreq * ::pow((maxFreq / minFreq), (bandIndex / (double) maxBands));

    return CenterFreq * (1 - skewFactor) + (minFreq + ((maxFreq - minFreq) * bandIndex * (1. / (double) maxBands))) * skewFactor;
}

/// <summary>
/// Scales the coefficient to a relative amplitude between 0.0 and 1.0.
/// </summary>
inline double ScaleA(double x)
{
    if (_Configuration.UseDecibels)
        return Map(ToDecibel(x), _Configuration.MinDecibels, _Configuration.MaxDecibels, 0.0, 1.0);

    double Exponent = 1.0 / _Configuration.Gamma;

    return Map(::pow(x, Exponent), _Configuration._UseAbsolute ? 0.0 : ::pow(ToMagnitude(_Configuration.MinDecibels), Exponent), ::pow(ToMagnitude(_Configuration.MaxDecibels), Exponent), 0.0, 1.0);
}


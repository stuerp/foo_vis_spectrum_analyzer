
/** $VER: Chrono.h (2024.08.16) P. Stuer - Implements a simple high-resolution chronometer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

#include <stdint.h>

class Chrono
{
public:
    /// <summary>
    /// Initializes this instance.
    /// </summary>
    Chrono()
    {
        if (!::QueryPerformanceFrequency((LARGE_INTEGER *) &_Frequency))
            _Frequency = 1;

        Reset();
    }

    /// <summary>
    /// Resets this instance.
    /// </summary>
    void Reset() noexcept
    {
        if (!::QueryPerformanceCounter((LARGE_INTEGER *) &_Last))
            _Last = 0;
    }

    /// <summary>
    /// Returns the number of seconds since the last reset.
    /// </summary>
    double Elapsed() const noexcept
    {
        int64_t Current;

        if (!::QueryPerformanceCounter((LARGE_INTEGER *) &Current))
            return 0;

        return (double) (Current - _Last) / (double) _Frequency;
    }

private:
    int64_t _Frequency; // Ticks per second
    int64_t _Last; // No. of ticks
};

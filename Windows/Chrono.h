
/** $VER: Chrono.h (2025.11.09) P. Stuer - Implements a simple high-resolution chronometer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

#include <stdint.h>

class chrono_t
{
public:
    /// <summary>
    /// Initializes this instance.
    /// </summary>
    chrono_t() noexcept
    {
        if (!::QueryPerformanceFrequency((LARGE_INTEGER *) &Frequency))
            Frequency = 1;

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

        return TicksToSeconds(Current - _Last);
    }

    /// <summary>
    /// Gets the current tick count.
    /// </summary>
    int64_t Now() const noexcept
    {
        LARGE_INTEGER Now = { };

        ::QueryPerformanceCounter(&Now);

        return Now.QuadPart;
    }

    /// <summary>
    /// Converts a tick count to seconds.
    /// </summary>
    double TicksToSeconds(int64_t ticks) const noexcept
    {
        return (double) ticks  / (double) Frequency;
    }

    /// <summary>
    /// Converts a tick count to milliseconds.
    /// </summary>
    int64_t TicksToMilliseconds(int64_t ticks) const noexcept
    {
        return ((ticks * 1'000) + (Frequency - 1)) / Frequency;
    }

    /// <summary>
    /// Converts a duration in seconds to ticks.
    /// </summary>
    int64_t SecondsToTicks(double seconds)
    {
        return (int64_t) (seconds * (double) Frequency);
    }

    /// <summary>
    /// Converts a duration in microseconds to ticks.
    /// </summary>
    int64_t MicrosecondsToTicks(int64_t microseconds)
    {
        return (microseconds * Frequency) / 1'000'000;
    }

public:
    int64_t Frequency; // Ticks per second

private:
    int64_t _Last; // No. of ticks
};

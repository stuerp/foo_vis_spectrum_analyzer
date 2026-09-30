
/** $VER: Chrono.h (2026.09.30) P. Stuer - Implements a simple high-resolution chronometer **/

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
    chrono_t() noexcept : _Last(Now()) { }

    /// <summary>
    /// Resets the starting point.
    /// </summary>
    void Reset() noexcept
    {
        _Last = Now();
    }

    /// <summary>
    /// Returns the number of seconds since the last reset.
    /// </summary>
    double Elapsed() const noexcept
    {
        return TicksToSeconds(Now() - _Last);
    }

    /// <summary>
    /// Gets the current tick count.
    /// </summary>
    static int64_t Now() noexcept
    {
        LARGE_INTEGER Value {};

        ::QueryPerformanceCounter(&Value);

        return Value.QuadPart;
    }

    /// <summary>
    /// Gets the performance-counter frequency in ticks per second.
    /// </summary>
    [[nodiscard]]
    static int64_t Frequency() noexcept
    {
        static const int64_t TheFrequency = []() noexcept
        {
            LARGE_INTEGER Value {};

            ::QueryPerformanceFrequency(&Value);

            return (Value.QuadPart > 0) ? Value.QuadPart : int64_t{ 1 };
        }();

        return TheFrequency;
    }

    /// <summary>
    /// Converts ticks to seconds.
    /// </summary>
    double TicksToSeconds(int64_t ticks) const noexcept
    {
        return (double) ticks  / (double) Frequency();
    }

    /// <summary>
    /// Converts ticks to milliseconds.
    /// </summary>
    int64_t TicksToMilliseconds(int64_t ticks) const noexcept
    {
        return ((ticks * 1'000) + (Frequency() - 1)) / Frequency();
    }

    /// <summary>
    /// Converts seconds to ticks.
    /// </summary>
    int64_t SecondsToTicks(double seconds)
    {
        return (int64_t) (seconds * (double) Frequency());
    }

    /// <summary>
    /// Converts microseconds to ticks.
    /// </summary>
    int64_t MicrosecondsToTicks(int64_t microseconds)
    {
        return (microseconds * Frequency()) / 1'000'000;
    }

private:
    int64_t _Last; // No. of ticks
};

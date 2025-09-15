
/**$VER: Event.h (2024.03.12) P. Stuer - Implements a very simple thread-safe event class. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

class event_t
{
public:
    /// <summary>
    /// Initializes a new instance.
    /// </summary>
    event_t()
    {
        Reset();
    }

    enum Flags
    {
        None = 0,

        PlaybackStartedNewTrack = 1,
        PlaybackStopped = 2,

        UserInterfaceColorsChanged = 4,
    };

    /// <summary>
    /// Resets this instance.
    /// </summary>
    void Reset()
    {
        ::InterlockedExchange64(&_Flags, 0);
    }

    /// <summary>
    /// Gets the current flags and resets them.
    /// </summary>
    event_t::Flags GetFlags() noexcept
    {
        return (event_t::Flags) ::InterlockedExchange64(&_Flags, 0);
    }

    /// <summary>
    /// Raises the specified flags.
    /// </summary>
    void Raise(event_t::Flags flags) noexcept
    {
        ::InterlockedOr64(&_Flags, flags);
    }

    /// <summary>
    /// Returns true if the flags in the specified mask are set.
    /// </summary>
    static bool IsRaised(event_t::Flags value, event_t::Flags mask) noexcept
    {
        return (value & mask) == mask;
    }

private:
    LONG64 _Flags;
};


/**$VER: Event.h (2026.08.17) P. Stuer - Implements a very simple thread-safe event class. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

/// <summary>
/// Implements a very simple thread-safe event class.
/// </summary>
class event_t final
{
public:
    /// <summary>
    /// Initializes a new instance.
    /// </summary>
    constexpr event_t() noexcept = default;

    event_t(const event_t &) = delete;
    event_t & operator=(const event_t &) = delete;

    enum Flags
    {
        None = 0,

        PlaybackNewTrack            = 1LL << 0,
        PlaybackStopped             = 1LL << 1,
        PlaybackPaused              = 1LL << 2,
        PlaybackResumed             = 1LL << 3,

        UserInterfaceColorsChanged  = 1LL << 4,
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
    Flags GetFlags() noexcept
    {
        return (Flags) ::InterlockedExchange64(&_Flags, 0);
    }

    /// <summary>
    /// Raises the specified flags.
    /// </summary>
    void Raise(Flags flags) noexcept
    {
        ::InterlockedOr64(&_Flags, flags);
    }

    /// <summary>
    /// Returns true if the flags in the specified mask are set.
    /// </summary>
    [[nodiscard]] static constexpr bool IsRaised(Flags value, Flags mask) noexcept
    {
        return (value & mask) != 0;
    }

    /// <summary>
    /// Returns true if every flag in the mask is set.
    /// </summary>
    [[nodiscard]] static constexpr bool IsAllRaised(Flags value, Flags mask) noexcept
    {
        const LONG64 maskValue = mask;

        return (value & maskValue) == maskValue;
    }

private:
    alignas(8) LONG64 _Flags = 0; // Interlocked 64-bit operations require appropriate alignment.
};

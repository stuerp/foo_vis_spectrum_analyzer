
/**$VER: Event.h (2024.03.11) P. Stuer - Implements a very simple mechanism between the UI and the render thread. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

class Event
{
public:
    Event() { Reset(); }

    enum Flags
    {
        None = 0,

        NewTrack,
        PlaybackStopped,
        ConfigurationChanged,
    };

    void Reset()
    {
        ::InterlockedExchange64(&_Flags, 0);
    }

    Event::Flags Get() noexcept
    {
        return (Event::Flags) ::InterlockedExchange64(&_Flags, 0);
    }

    void Raise(Event::Flags flags) noexcept
    {
        ::InterlockedOr64(&_Flags, flags);
    }

    static bool IsRaised(Event::Flags value, Event::Flags flags) noexcept
    {
        return (value & flags) == flags;
    }

private:
    LONG64 _Flags;
};


/** $VER: BaseProcessor.h (2026.11.05) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include "State.h"

/// <summary>
/// Represents a meter measurement.
/// </summary>
struct measurement_t
{
    measurement_t(Channels channel, const WCHAR * _channelName) noexcept : Channel(channel), ChannelName(_channelName) { }

    Channels Channel;
    std::wstring ChannelName;
};

enum class ResetOptions
{
    Value = 1 << 0,
    Peak  = 1 << 1,

    All   = Value | Peak
};

template <typename Enum> constexpr auto ToUnderlying(Enum value) noexcept
{
    static_assert(std::is_enum_v<Enum>);

    return static_cast<std::underlying_type_t<Enum>>(value);
}

constexpr bool IsSet(ResetOptions options, ResetOptions flag) noexcept
{
    return (ToUnderlying(options) & ToUnderlying(flag)) != 0;
}

constexpr bool has_all_flags(ResetOptions options, ResetOptions flags) noexcept
{
    const auto Flags = ToUnderlying(flags);

    return (ToUnderlying(options) & Flags) == Flags;
}

/// <summary>
/// Represents the analysis of the sample data.
/// </summary>
class audio_processor_t
{
public:
    audio_processor_t(state_t & state) noexcept : _State(state), _SampleRate(), _ChannelCount(), _ChannelConfig() { };

    audio_processor_t(const audio_processor_t &) = delete;
    audio_processor_t(audio_processor_t &&) = delete;

    audio_processor_t & operator=(const audio_processor_t &) = delete;
    audio_processor_t & operator=(audio_processor_t &&) = delete;

    virtual ~audio_processor_t() noexcept { Reset(); };

    virtual void Configure(const graph_options_t * options) noexcept;
    virtual void Process(const audio_chunk & chunk) noexcept;
    virtual void AnimatePeaks(bool isStopped) noexcept;
    virtual void Reset() noexcept;

    virtual void ResetMeasurements(ResetOptions options = ResetOptions::All) noexcept { }

    static std::unique_ptr<audio_processor_t> Create(state_t & state) noexcept;

public:
    const state_t & _State;
    const graph_options_t * _Options;

    static const uint32_t ChannelPairs[6];

protected:
    uint32_t _SampleRate;
    uint32_t _ChannelCount;
    uint32_t _ChannelConfig;
    uint32_t _ChannelMask;

    msc::chrono_t _Chrono;
};

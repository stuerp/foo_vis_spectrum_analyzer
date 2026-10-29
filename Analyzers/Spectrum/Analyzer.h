
/** $VER: Analyzer.h (2026.10.27) P. Stuer - Base class for all spectrum analyzers **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "State.h"
#include "WindowFunctions.h"
#include "FrequencyBand.h"

class analyzer_t
{
public:
    analyzer_t() = delete;

    analyzer_t(const analyzer_t &) = delete;
    analyzer_t(analyzer_t &&) = delete;

    analyzer_t & operator=(const analyzer_t &) = delete;
    analyzer_t & operator=(analyzer_t &&) = delete;

    virtual ~analyzer_t() = default;

    /// <summary>
    /// Initializes a new instance.
    /// </summary>
    analyzer_t(const state_t & state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelConfig, const window_function_t & windowFunction) : _State(state), WindowFunction(windowFunction), _SampleRate(sampleRate), _ChannelCount(channelCount), _ChannelConfig(channelConfig)
    {
        NyquistFrequency = (double) _SampleRate / 2.;
    }

    virtual bool AnalyzeAudio(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept = 0;

    /// <summary>
    /// Downmixes the selected channels of an audio frame to a mono sample.
    /// </summary>
    audio_sample Downmix(const audio_sample * frame, const uint32_t selectedChannels) const noexcept
    {
        if (frame == nullptr)
            return audio_sample {};

        uint32_t AvailableChannels = _ChannelConfig;

        const uint32_t ChannelsToMix = AvailableChannels & selectedChannels;

        if (ChannelsToMix == 0)
            return audio_sample {};

        double Sum = 0.;

        while (AvailableChannels != 0)
        {
            // Isolate the least-significant configured channel.
            const uint32_t Channel = AvailableChannels & (0u - AvailableChannels);

            if ((ChannelsToMix & Channel) != 0)
                Sum += static_cast<double>(*frame);

            ++frame;

            AvailableChannels &= AvailableChannels - 1; // Remove lowest set bit.
        }

        const auto ChannelCount = std::popcount(ChannelsToMix);

        return (audio_sample) (Sum / (double) ChannelCount);
    }

protected:
    const state_t & _State;
    const window_function_t & WindowFunction;

    uint32_t _SampleRate;
    uint32_t _ChannelCount;  // Number of channels per frame.
    uint32_t _ChannelConfig; // Mask representing the channels present in the frame.

    double NyquistFrequency;
};

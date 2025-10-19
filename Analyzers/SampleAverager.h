
/** $VER: SamplerAverager.h (2025.10.19) P. Stuer - Implements a sample averager scaler as a functor **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <audio_math.h>
#include <cmath>

/// <summary>
/// Implements a sample averager scaler as a functor.
/// </summary>
class sample_averager_t
{
public:
    explicit sample_averager_t(uint32_t channelCount, uint32_t selectedChannels) noexcept : _ChannelCount(channelCount), _SelectedChannels(selectedChannels)
    {
    }

    /// <summary>
    /// Averages the samples of the specified frame.
    /// </summary>
    inline audio_sample operator()(const audio_sample * samples) const noexcept
    {
        audio_sample Average = 0.;
        uint32_t n = 0;
        uint32_t SelectedChannels = _SelectedChannels;

        for (uint32_t i = 0; (i < _ChannelCount) && (SelectedChannels != 0); ++i, ++samples, SelectedChannels >>= 1)
        {
            if (SelectedChannels & 1)
            {
                Average += *samples;
                n++;
            }
        }

        return (n > 0) ? Average / (audio_sample) n : (audio_sample) 0.;
    }

private:
    uint32_t _ChannelCount;
    uint32_t _SelectedChannels;
};

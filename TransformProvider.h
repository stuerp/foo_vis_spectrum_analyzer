
/** $VER: TransformProvider.h (2023.12.02) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

using namespace std;

/// <summary>
/// Provides a base class for transform providers.
/// </summary>
class TransformProvider
{
public:
    audio_sample AverageSamples(const audio_sample * samples, uint32_t channelMask) const noexcept;

protected:
    uint32_t _ChannelCount;
    uint32_t _ChannelSetup;
};

/// <summary>
/// Calculates the average of the specified samples.
/// </summary>
inline audio_sample TransformProvider::AverageSamples(const audio_sample * samples, uint32_t channelMask) const noexcept
{
    audio_sample Average = 0.;
    uint32_t n = 0;

    for (uint32_t i = 0; (i < _ChannelCount) && (channelMask != 0); ++i, channelMask >>= 1)
    {
        if (channelMask & 1)
        {
            Average += *samples;
            n++;
        }
    }

    return (n > 0) ? Average / (audio_sample) n : (audio_sample) 0.;

/*
    switch (_ChannelCount)
    {
        case 0:
            return 0.; // Should not happen.

        case 1:
            return samples[i];

        case 2:
            return (samples[i] + samples[i + 1]) / (audio_sample) 2.0;

        case 3:
            return (samples[i] + samples[i + 1] + samples[i + 2]) / (audio_sample) 3.0;

        case 4:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3]) / (audio_sample) 4.0;

        case 5:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4]) / (audio_sample) 5.0;

        case 6:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4] + samples[i + 5]) / (audio_sample) 6.0;

        default:
        {
            audio_sample Average = 0.;

            for (uint32_t j = 0; j < _ChannelCount; ++j)
                Average += samples[i + j];

            return Average / (audio_sample) _ChannelCount;
        }
    }
*/
}

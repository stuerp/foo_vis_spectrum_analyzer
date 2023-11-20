
/** $VER: TransformProvider.h (2023.11.20) P. Stuer **/

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
    static audio_sample AverageSamples(const audio_sample * samples, size_t i, size_t channelCount);
};

/// <summary>
/// Calculates the average of the specified samples.
/// </summary>
inline audio_sample TransformProvider::AverageSamples(const audio_sample * samples, size_t i, size_t channelCount)
{
    switch (channelCount)
    {
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

            for (size_t j = 0; j < channelCount; ++j)
                Average += samples[i + j];

            return Average / (audio_sample) channelCount;
        }
    }
}


/** $VER: TransformProvider.h (2023.12.30) P. Stuer **/

#pragma once

#include "framework.h"

using namespace std;

#include "WindowFunctions.h"

/// <summary>
/// Provides a base class for transform providers.
/// </summary>
class TransformProvider
{
public:
    TransformProvider() = delete;

    TransformProvider(const TransformProvider &) = delete;
    TransformProvider & operator=(const TransformProvider &) = delete;
    TransformProvider(TransformProvider &&) = delete;
    TransformProvider & operator=(TransformProvider &&) = delete;

    virtual ~TransformProvider() { }

    /// <summary>
    /// Initializes a new instance.
    /// </summary>
    TransformProvider(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction) : _ChannelCount(channelCount), _ChannelSetup(channelSetup), _SampleRate(sampleRate), _WindowFunction(windowFunction)
    {
        _NyquistFrequency = _SampleRate / 2.;
    }

    /// <summary>
    /// Calculates the average of the specified samples.
    /// </summary>
    audio_sample AverageSamples(const audio_sample * samples, uint32_t channelMask) const noexcept
    {
        audio_sample Average = 0.;
        uint32_t n = 0;

        for (uint32_t i = 0; (i < _ChannelCount) && (channelMask != 0); ++i, ++samples, channelMask >>= 1)
        {
            if (channelMask & 1)
            {
                Average += *samples;
                n++;
            }
        }

        return (n > 0) ? Average / (audio_sample) n : (audio_sample) 0.;
    }

protected:
    uint32_t _ChannelCount;
    uint32_t _ChannelSetup;
    double _SampleRate;
    const WindowFunction & _WindowFunction;

    double _NyquistFrequency;
};

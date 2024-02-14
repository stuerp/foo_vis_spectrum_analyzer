
/** $VER: Analyzer.h (2024.02.13) P. Stuer **/

#pragma once

#include "framework.h"

using namespace std;

#include "State.h"
#include "WindowFunctions.h"

/// <summary>
/// Provides a base class for analyzers.
/// </summary>
class Analyzer
{
public:
    Analyzer() = delete;

    Analyzer(const Analyzer &) = delete;
    Analyzer & operator=(const Analyzer &) = delete;
    Analyzer(Analyzer &&) = delete;
    Analyzer & operator=(Analyzer &&) = delete;

    virtual ~Analyzer() { }

    /// <summary>
    /// Initializes a new instance.
    /// </summary>
    Analyzer(const State * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction) : _State(configuration), _SampleRate(sampleRate), _ChannelCount(channelCount), _ChannelSetup(channelSetup), _WindowFunction(windowFunction)
    {
        _NyquistFrequency = (double) _SampleRate / 2.;
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
    const State * _State;
    uint32_t _SampleRate;
    uint32_t _ChannelCount;
    uint32_t _ChannelSetup;
    const WindowFunction & _WindowFunction;

    double _NyquistFrequency;
};

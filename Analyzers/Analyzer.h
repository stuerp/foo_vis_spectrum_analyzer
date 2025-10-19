
/** $VER: Analyzer.h (2025.10.05) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

using namespace std;

#include "State.h"
#include "WindowFunctions.h"

/// <summary>
/// Provides a base class for analyzers.
/// </summary>
#pragma warning(disable: 4820)
class analyzer_t
{
public:
    analyzer_t() = delete;

    analyzer_t(const analyzer_t &) = delete;
    analyzer_t & operator=(const analyzer_t &) = delete;
    analyzer_t(analyzer_t &&) = delete;
    analyzer_t & operator=(analyzer_t &&) = delete;

    virtual ~analyzer_t() { }

    /// <summary>
    /// Initializes a new instance.
    /// </summary>
    analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction) : _State(state), _SampleRate(sampleRate), _ChannelCount(channelCount), _ChannelSetup(channelSetup), _WindowFunction(windowFunction)
    {
        _NyquistFrequency = (double) _SampleRate / 2.;
    }

    /// <summary>
    /// Calculates the average of the samples of the specified audio frame.
    /// </summary>
    audio_sample AverageSamples(const audio_sample * samples, uint32_t selectedChannels) const noexcept
    {
        audio_sample Average = 0.;
        uint32_t n = 0;

        for (uint32_t i = 0; (i < _ChannelCount) && (selectedChannels != 0); ++i, ++samples, selectedChannels >>= 1)
        {
            if (selectedChannels & 1)
            {
                Average += *samples;
                n++;
            }
        }

        return (n > 0) ? Average / (audio_sample) n : (audio_sample) 0.;
    }

protected:
    const state_t * _State;
    uint32_t _SampleRate;
    uint32_t _ChannelCount; // Number of channels per frame.
    uint32_t _ChannelSetup; // Mask representing the channels present in the frame.
    const window_function_t & _WindowFunction;

    double _NyquistFrequency;
};

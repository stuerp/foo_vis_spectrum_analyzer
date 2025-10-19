
/** $VER: CQTAnalyzer.cpp (2025.10.19) P. Stuer - Based on TF3RDL's Constant-Q analyzer, https://codepen.io/TF3RDL/pen/poQJwRW **/

#include "pch.h"
#include "CQTAnalyzer.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
cqt_analyzer_t::cqt_analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction) : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction)
{
}

/// <summary>
/// Calculates the Constant-Q Transform on the sample data and returns the frequency bands using the Goertzel transform.
/// </summary>
bool cqt_analyzer_t::AnalyzeSamples(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept
{
    const size_t SampleCount  = frameCount * _ChannelCount;
    const double SampleDuration = (double) _SampleRate / (double) SampleCount;

    const bool UseGranularSamplingPeriod = false;
    const bool UseGranularBandwidth = true;

    for (frequency_band_t & fb : frequencyBands)
    {
        const double Bandwidth  = std::abs(fb.Hi - fb.Lo) + (SampleDuration * _State->_CQTBandwidthOffset);
        const double TimeLength = std::min(1. / Bandwidth, 1. / SampleDuration);

        double SamplingPeriod = std::max(1., std::trunc(((double) _SampleRate * _State->_CQTDownSample) / (fb.Center + TimeLength)));

        if (!UseGranularSamplingPeriod)
            SamplingPeriod = std::pow(2., std::trunc(std::log2(SamplingPeriod)));

        const double KTerm = fb.Center * SamplingPeriod;                // Frequency of interest
        const double Omega = 2. * M_PI * KTerm / (double) _SampleRate;  // ω
        const double Coeff = 2. * std::cos(Omega);

        double BandSampleCount = TimeLength * (double) _SampleRate * _ChannelCount;

        if (!UseGranularBandwidth)
            BandSampleCount = std::min(std::trunc(std::pow(2., std::round(std::log2(BandSampleCount)))), (double) SampleCount);

        const double Offset = std::trunc(((double) SampleCount - BandSampleCount) * (0.5 + _State->_CQTAlignment / 2.));

        const double LoIdx = Offset;
        const double HiIdx = LoIdx + std::trunc(BandSampleCount) - 1.;

        double f1 = 0.;
        double f2 = 0.;

        double Norm = 0.;

        #pragma loop(hint_parallel(2))
        for (double Idx = std::trunc(LoIdx / SamplingPeriod); Idx <= std::trunc(HiIdx / SamplingPeriod); Idx += _ChannelCount)
        {
            const double x = (((Idx * SamplingPeriod) - LoIdx) / (HiIdx - LoIdx) * 2.) - 1.;
            const double w = _WindowFunction(x);

            const size_t i = (size_t) (Idx * SamplingPeriod);

            const double s = ((i < SampleCount - _ChannelCount) ? (AverageSamples(&frames[i], selectedChannels) * w) : 0.) + (Coeff * f1) - f2;

            Norm += w;

            f2 = f1;
            f1 = s;
        }

        fb.NewValue = std::sqrt((f1 * f1) + (f2 * f2) - (Coeff * f1 * f2)) / Norm; // Power
    }

    return true;
}

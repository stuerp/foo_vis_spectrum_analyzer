
/** $VER: CQTAnalyzer.cpp (2024.02.17) P. Stuer - Based on TF3RDL's Constant-Q analyzer, https://codepen.io/TF3RDL/pen/poQJwRW **/

#include "CQTAnalyzer.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
CQTAnalyzer::CQTAnalyzer(const State * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction) : Analyzer(state, sampleRate, channelCount, channelSetup, windowFunction)
{
}

/// <summary>
/// Calculates the Constant-Q Transform on the sample data and returns the frequency bands.
/// </summary>
bool CQTAnalyzer::AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept
{
    for (FrequencyBand & fb : frequencyBands)
    {
        double Bandwidth = ::fabs(fb.Hi - fb.Lo) + ((double) _SampleRate / (double) sampleCount) * _State->_CQTBandwidthOffset;
        double TLen = Min(1. / Bandwidth, (double) sampleCount / (double) _SampleRate);

        double DownsampleAmount = Max(1.0, ::trunc(((double) _SampleRate * _State->_CQTDownSample) / (fb.Ctr + TLen)));
        double Coeff = 2. * ::cos(2. * M_PI * fb.Ctr / (double) _SampleRate * DownsampleAmount);

        double f1 = 0.;
        double f2 = 0.;
        double Sine = 0.;
        double Offset = ::trunc(((double) sampleCount - TLen * (double) _SampleRate) * (0.5 + _State->_CQTAlignment / 2.));

        double LoIdx = Offset;
        double HiIdx = ::trunc(TLen * (double) _SampleRate) + Offset - 1.;
        double Norm = 0.;

        #pragma loop(hint_parallel(2))
        for (double Idx = ::trunc(LoIdx / DownsampleAmount); Idx <= ::trunc(HiIdx / DownsampleAmount); ++Idx)
        {
            double x = ((Idx * DownsampleAmount - LoIdx) / (HiIdx - LoIdx) * 2. - 1.);

            double w = _WindowFunction(x);

            Norm += w;

            // Goertzel transform
            Sine = (AverageSamples(&sampleData[(size_t)(Idx * DownsampleAmount)], channels) * w) + (Coeff * f1) - f2;

            f2 = f1;
            f1 = Sine;
        }

        fb.NewValue = ::sqrt((f1 * f1) + (f2 * f2) - Coeff * f1 * f2) / Norm;
    }

    return true;
}

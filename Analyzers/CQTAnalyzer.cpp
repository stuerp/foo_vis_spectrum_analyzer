
/** $VER: CQTAnalyzer.cpp (2024.01.02) P. Stuer **/

#include "CQTAnalyzer.h"

#include "Support.h"

#include <complex>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
CQTAnalyzer::CQTAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, double bandwidthOffset, double alignment, double downSample, const Configuration * configuration) : TransformProvider(channelCount, channelSetup, sampleRate, windowFunction)
{
    _BandwidthOffset = bandwidthOffset;
    _Alignment = alignment;
    _DownSample = downSample;
}

/// <summary>
/// Calculates the Constant-Q Transform on the sample data and returns the frequency bands.
/// </summary>
bool CQTAnalyzer::GetFrequencyBands(const audio_sample * sampleData, size_t sampleCount, uint32_t channelMask, vector<FrequencyBand> & frequencyBands) const
{
    for (FrequencyBand & Iter : frequencyBands)
    {
        double Bandwidth = ::fabs(Iter.Hi - Iter.Lo) + (_SampleRate / (double) sampleCount) * _BandwidthOffset;
        double TLen = Min(1. / Bandwidth, (double) sampleCount / _SampleRate);

        double DownsampleAmount = Max(1.0, ::trunc((_SampleRate * _DownSample) / (Iter.Ctr + TLen)));
        double Coeff = 2. * ::cos(2. * M_PI * Iter.Ctr / _SampleRate * DownsampleAmount);

        double f1 = 0.;
        double f2 = 0.;
        double Sine = 0.;
        double Offset = ::trunc(((double) sampleCount - TLen * _SampleRate) * (0.5 + _Alignment / 2.));

        double LoIdx = Offset;
        double HiIdx = ::trunc(TLen * _SampleRate) + Offset - 1.;
        double Norm = 0.;

        for (double Idx = ::trunc(LoIdx / DownsampleAmount); Idx <= ::trunc(HiIdx / DownsampleAmount); ++Idx)
        {
            double x = ((Idx * DownsampleAmount - LoIdx) / (HiIdx - LoIdx) * 2. - 1.);

            double w = _WindowFunction(x);

            Norm += w;

            // Goertzel transform
            Sine = (AverageSamples(&sampleData[(size_t)(Idx * DownsampleAmount)], channelMask) * w) + (Coeff * f1) - f2;

            f2 = f1;
            f1 = Sine;
        }

        Iter.NewValue = ::sqrt((f1 * f1) + (f2 * f2) - Coeff * f1 * f2) / Norm;
    }

    return true;
}

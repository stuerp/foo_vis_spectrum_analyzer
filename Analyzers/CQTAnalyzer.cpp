
/** $VER: CQTAnalyzer.cpp (2024.02.13) P. Stuer **/

#include "CQTAnalyzer.h"

#include "Support.h"

#include <complex>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
CQTAnalyzer::CQTAnalyzer(const Configuration * configuration, double sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction) : Analyzer(configuration, sampleRate, channelCount, channelSetup, windowFunction)
{
}

/// <summary>
/// Calculates the Constant-Q Transform on the sample data and returns the frequency bands.
/// </summary>
bool CQTAnalyzer::AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & frequencyBands) const
{
    for (FrequencyBand & Iter : frequencyBands)
    {
        double Bandwidth = ::fabs(Iter.Hi - Iter.Lo) + (_SampleRate / (double) sampleCount) * _Configuration->_CQTBandwidthOffset;
        double TLen = Min(1. / Bandwidth, (double) sampleCount / _SampleRate);

        double DownsampleAmount = Max(1.0, ::trunc((_SampleRate * _Configuration->_CQTDownSample) / (Iter.Ctr + TLen)));
        double Coeff = 2. * ::cos(2. * M_PI * Iter.Ctr / _SampleRate * DownsampleAmount);

        double f1 = 0.;
        double f2 = 0.;
        double Sine = 0.;
        double Offset = ::trunc(((double) sampleCount - TLen * _SampleRate) * (0.5 + _Configuration->_CQTAlignment / 2.));

        double LoIdx = Offset;
        double HiIdx = ::trunc(TLen * _SampleRate) + Offset - 1.;
        double Norm = 0.;

        for (double Idx = ::trunc(LoIdx / DownsampleAmount); Idx <= ::trunc(HiIdx / DownsampleAmount); ++Idx)
        {
            double x = ((Idx * DownsampleAmount - LoIdx) / (HiIdx - LoIdx) * 2. - 1.);

            double w = _WindowFunction(x);

            Norm += w;

            // Goertzel transform
            Sine = (AverageSamples(&sampleData[(size_t)(Idx * DownsampleAmount)], _Configuration->_SelectedChannels) * w) + (Coeff * f1) - f2;

            f2 = f1;
            f1 = Sine;
        }

        Iter.NewValue = ::sqrt((f1 * f1) + (f2 * f2) - Coeff * f1 * f2) / Norm;
    }

    return true;
}

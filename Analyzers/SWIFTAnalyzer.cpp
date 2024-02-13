
/** $VER: SWIFTAnalyzer.cpp (2024.02.13) P. Stuer - Based on TF3RDL Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

#include "SWIFTAnalyzer.h"

#include "Support.h"

#include <algorithm>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
SWIFTAnalyzer::SWIFTAnalyzer(const Configuration * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup) : Analyzer(configuration, sampleRate, channelCount, channelSetup, WindowFunction())
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
bool SWIFTAnalyzer::Initialize(const vector<FrequencyBand> & frequencyBands)
{
    const double Factor = 4. * _Configuration->_SWIFTBandwidth / (double) _SampleRate - 1. / (_Configuration->_TimeResolution * (double) _SampleRate / 2000.);

    // Note: x and y are used instead of real and imaginary numbers since vector rotation is the equivalent of the complex one.
    for (const FrequencyBand & Iter : frequencyBands)
    {
        // Pre-calculate rX and rY here since sin and cos functions are pretty slow.
        Coef c =
        {
            ::cos(Iter.Ctr * M_PI * 2. / (double) _SampleRate),
            ::sin(Iter.Ctr * M_PI * 2. / (double) _SampleRate),
            ::exp(-::abs(Iter.Hi - Iter.Lo) * Factor),
        };

        c.Values.resize(_Configuration->_FilterBankOrder, { 0., 0.});

        _Coefs.push_back(c);
    }

    return true;
}

/// <summary>
/// Calculates the transform and returns the frequency bands.
/// </summary>
bool SWIFTAnalyzer::AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & frequencyBands)
{
    for (auto & Iter : frequencyBands)
        Iter.NewValue = 0.;

    for (size_t i = 0; i < sampleCount; i += _ChannelCount)
    {
        const audio_sample Sample = AverageSamples(&sampleData[i], _Configuration->_SelectedChannels);

        size_t k = 0;

        for (auto & Coef : _Coefs)
        {
            size_t j = 0;
            Value OldValue = { };

            for (auto & value : Coef.Values)
            {
                Value NewValue;

                if (j == 0)
                    NewValue = { Sample, 0. };
                else
                    NewValue = OldValue;

                value =
                {
                    (value.x * Coef.rX - value.y * Coef.rY) * Coef.Decay + NewValue.x * (1. - Coef.Decay),
                    (value.x * Coef.rY + value.y * Coef.rX) * Coef.Decay + NewValue.y * (1. - Coef.Decay)
                };

                OldValue = value;
                j++;
            }

            // OldValue now contains the last calculated value.
            frequencyBands[k].NewValue = Max(frequencyBands[k].NewValue, (OldValue.x * OldValue.x) + (OldValue.y * OldValue.y));
            k++;
        }
    }

    for (auto & Iter : frequencyBands)
        Iter.NewValue = ::sqrt(Iter.NewValue);

    return true;
}

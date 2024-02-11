
/** $VER: SWIFTAnalyzer.h (2024.02.11) P. Stuer - Based on TF3RDL Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

#include "SWIFTAnalyzer.h"

#include "Support.h"

#include <algorithm>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
SWIFTAnalyzer::SWIFTAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction) : TransformProvider(channelCount, channelSetup, sampleRate, windowFunction)
{
    _spectrumData = nullptr;
}

/// <summary>
/// Initializes this instance.
/// </summary>
bool SWIFTAnalyzer::Initialize(const vector<FrequencyBand> & frequencyBands, size_t order, double timeResolution, double bandwidth)
{
    const double Factor = 4. * bandwidth / _SampleRate - 1. / (timeResolution * _SampleRate / 2000.);

    // Note: x and y are used instead of real and imaginary numbers since vector rotation is the equivalent of the complex one.
    for (const FrequencyBand & Iter : frequencyBands)
    {
        // Pre-calculate rX and rY here since sin and cos functions are pretty slow.
        Coef c =
        {
            ::cos(Iter.Ctr * M_PI * 2. / _SampleRate),
            ::sin(Iter.Ctr * M_PI * 2. / _SampleRate),
            ::exp(-::abs(Iter.Hi - Iter.Lo) * Factor),
        };

        c.coeffs.resize(order, { 0., 0.});

        _Coefs.push_back(c);
    }

    return true;
}

/// <summary>
/// Calculates the transform and returns the frequency bands.
/// </summary>
bool SWIFTAnalyzer::AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channelMask, vector<FrequencyBand> & frequencyBands)
{
    for (auto & Iter : frequencyBands)
        Iter.NewValue = 0.;

    for (size_t i = 0; i < sampleCount; i += _ChannelCount)
    {
        audio_sample Sample = AverageSamples(&sampleData[i], channelMask);

        size_t k = 0;

        for (auto & c : _Coefs)
        {
            size_t j = 0;
            Point OldInput = { };

            for (auto & p : c.coeffs)
            {
                Point input;

                if (j == 0)
                    input = { Sample, 0. };
                else
                    input = OldInput;

                p =
                {
                    (p.x * c.rX - p.y * c.rY) * c.decay + input.x * (1. - c.decay),
                    (p.x * c.rY + p.y * c.rX) * c.decay + input.y * (1. - c.decay)
                };

                OldInput = p;
                j++;
            }

            frequencyBands[k].NewValue = Max(frequencyBands[k].NewValue, (OldInput.x * OldInput.x) + (OldInput.y * OldInput.y));
            k++;
        }
    }

    for (auto & Iter : frequencyBands)
        Iter.NewValue = ::sqrt(Iter.NewValue);

    return true;
}

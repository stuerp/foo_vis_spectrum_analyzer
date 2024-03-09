
/** $VER: SWIFTAnalyzer.cpp (2024.03.02) P. Stuer - Based on TF3RDL's Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

#include "SWIFTAnalyzer.h"

#include "Support.h"

#include <algorithm>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
SWIFTAnalyzer::SWIFTAnalyzer(const State * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup) : Analyzer(state, sampleRate, channelCount, channelSetup, WindowFunction())
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
bool SWIFTAnalyzer::Initialize(const vector<FrequencyBand> & frequencyBands)
{
    const double Factor1 = M_PI * _State->_IIRBandwidth / (double) _SampleRate - 1. / (_State->_TimeResolution * (double) _SampleRate / (M_PI * 1000.));
    const double Factor2 = _State->_CompensateBW ? ::sqrt(_State->_FilterBankOrder) : 1.;

    // Note: x and y are used instead of real and imaginary numbers since vector rotation is the equivalent of the complex one.
    for (const FrequencyBand & fb : frequencyBands)
    {
        // Pre-calculate rX and rY here since sin and cos functions are pretty slow.
        Coef c =
        {
            ::cos(fb.Ctr * M_PI * 2. / (double) _SampleRate),
            ::sin(fb.Ctr * M_PI * 2. / (double) _SampleRate),
            ::exp(-::abs(fb.Hi - fb.Lo) * Factor1 * Factor2)
        };

        for (uint32_t i = 0; i < _State->_FilterBankOrder; ++i)
            c.Values[i] = { };

        _Coefs.push_back(c);
    }

    return true;
}

/// <summary>
/// Calculates the transform and returns the frequency bands.
/// </summary>
bool SWIFTAnalyzer::AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept
{
    for (auto & fb : frequencyBands)
        fb.NewValue = 0.;

    #pragma loop(hint_parallel(2))
    for (size_t i = 0; i < sampleCount; i += _ChannelCount)
    {
        const audio_sample Sample = AverageSamples(&sampleData[i], channels);

        size_t k = 0;

        for (auto & Coef : _Coefs)
        {
            Value CurValue = { Sample, 0. };

            for (uint32_t j = 0; j < _State->_FilterBankOrder; ++j)
            {
                Value & value = Coef.Values[j];

                value =
                {
                    (value.x * Coef.rX - value.y * Coef.rY) * Coef.Decay + CurValue.x * (1. - Coef.Decay),
                    (value.x * Coef.rY + value.y * Coef.rX) * Coef.Decay + CurValue.y * (1. - Coef.Decay)
                };

                CurValue = value;
            }

            // CurValue now contains the last calculated value.
            frequencyBands[k].NewValue = Max(frequencyBands[k].NewValue, (CurValue.x * CurValue.x) + (CurValue.y * CurValue.y));
            k++;
        }
    }

    for (auto & fb : frequencyBands)
        fb.NewValue = ::sqrt(fb.NewValue);

    return true;
}

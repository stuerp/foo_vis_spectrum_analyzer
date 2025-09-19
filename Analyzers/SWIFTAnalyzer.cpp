
/** $VER: SWIFTAnalyzer.cpp (2025.09.15) P. Stuer - Based on TF3RDL's Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

#include "pch.h"
#include "SWIFTAnalyzer.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
swift_analyzer_t::swift_analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup) : analyzer_t(state, sampleRate, channelCount, channelSetup, window_function_t())
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
bool swift_analyzer_t::Initialize(const frequency_bands_t & frequencyBands) noexcept
{
    const double Factor = 4. * _State->_IIRBandwidth / (double) _SampleRate - 1. / (_State->_TimeResolution * (double) _SampleRate / 2000.);

    const double a = M_PI * 2. / (double) _SampleRate;

    // Note: x and y are used instead of real and imaginary numbers since vector rotation is the equivalent of the complex one. Pre-calculate rX and rY here since sin and cos functions are pretty slow.
    for (const frequency_band_t & fb : frequencyBands)
        _Coefs.push_back(swift_coef_t(::cos(fb.Ctr * a), ::sin(fb.Ctr * a), ::exp(-::abs(fb.Hi - fb.Lo) * Factor)));

    return true;
}

/// <summary>
/// Calculates the transform and returns the updated frequency bands.
/// </summary>
bool swift_analyzer_t::AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, frequency_bands_t & frequencyBands) noexcept
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
            swift_value_t CurValue = { Sample, 0. };

            for (uint32_t j = 0; j < _State->_FilterBankOrder; ++j)
            {
                swift_value_t & Value = Coef.Values[j];

                Value =
                {
                    (Value.x * Coef.rX - Value.y * Coef.rY) * Coef.Decay + CurValue.x * (1. - Coef.Decay),
                    (Value.x * Coef.rY + Value.y * Coef.rX) * Coef.Decay + CurValue.y * (1. - Coef.Decay)
                };

                CurValue = Value;
            }

            // CurValue now contains the last calculated value.
            frequencyBands[k].NewValue = std::max(frequencyBands[k].NewValue, (CurValue.x * CurValue.x) + (CurValue.y * CurValue.y));
            k++;
        }
    }

    for (auto & fb : frequencyBands)
        fb.NewValue = ::sqrt(fb.NewValue);

    return true;
}

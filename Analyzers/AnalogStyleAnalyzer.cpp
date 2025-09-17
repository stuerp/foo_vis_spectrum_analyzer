
/** $VER: AnalogStyleAnalyzer.cpp (2024.03.02) P. Stuer - Based on TF3RDL's Analog-style spectrum analyzer, https://codepen.io/TF3RDL/pen/MWLzPoO **/

#include "pch.h"

#include "AnalogStyleAnalyzer.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
analog_style_analyzer_t::analog_style_analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction) : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction)
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
bool analog_style_analyzer_t::Initialize(const vector<frequency_band_t> & frequencyBands)
{
    assert(_SampleRate != 0);

    const double TimeResolution =  _State->_ConstantQ ? std::numeric_limits<double>::infinity() : _State->_TimeResolution;

    for (const frequency_band_t & fb : frequencyBands)
    {
        // Biquad bandpass filter. Cascaded biquad bandpass is not Butterworth nor Bessel, rather it is something called "critically-damped" since each filter stage shares the same every biquad coefficients.
        const double rad = M_PI * fb.Ctr / (double) _SampleRate;

        const double K = std::tan(rad);
        const double Bandwidth = std::abs(fb.Hi - fb.Lo) * _State->_IIRBandwidth + (1. / (TimeResolution / 1000.));

        const double QCompensationFactor = _State->_PreWarpQ ? rad / K : 1.;
        const double Q = fb.Ctr / Bandwidth * QCompensationFactor / (_State->_CompensateBW ? ::sqrt(_State->_FilterBankOrder) : 1.);
        const double Norm = 1 / (1 + K / Q + K * K);

        coef_t c = { };

        c.a0 = K / Q * Norm;
        c.a1 = 0.;
        c.a2 = -c.a0;
        c.b1 = 2. * (K * K - 1.)    * Norm;
        c.b2 = (1. - K / Q + K * K) * Norm;

        for (uint32_t i = 0; i < _State->_FilterBankOrder; ++i)
            c.z1[i] = c.z2[i] = c.Out[i] = 0.f;

        _Coefs.push_back(c);
    }

    return true;
}

/// <summary>
/// Calculates the Constant-Q Transform on the sample data and returns the frequency bands.
/// </summary>
bool analog_style_analyzer_t::AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, frequency_bands_t & frequencyBands) noexcept
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
            double Value = (double) Sample;

            for (uint32_t j = 0; j < _State->_FilterBankOrder;)
            {
                Coef.Out[j] = (Value * Coef.a0) + Coef.z1[j];
                Coef.z1[j]  = (Value * Coef.a1) + Coef.z2[j] - (Coef.b1 * Coef.Out[j]);
                Coef.z2[j]  = (Value * Coef.a2)              - (Coef.b2 * Coef.Out[j]);

                ++j;

                Value = Coef.Out[j - 1];
            }

            frequencyBands[k].NewValue = std::max(frequencyBands[k].NewValue, ::abs(Value));
            ++k;
        }
    }

    return true;
}

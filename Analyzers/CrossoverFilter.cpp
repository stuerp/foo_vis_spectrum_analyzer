
/** $VER: CrossoverFilter.cpp (2026.09.21) P. Stuer - Implements an audio crossover filter. **/

#include <pch.h>

#include "CrossoverFilter.h"

/// <summary>
/// Configure this instance.
/// </summary>
HRESULT biquad_t::Configure(PassType type, double frequency, double sampleRate, double q) noexcept
{
    constexpr double TwoPi = 2. * std::numbers::pi_v<double>;

    // Prevent invalid or unstable cutoff frequencies. The cutoff frequency is between 10Hz and the Nyquist frequency.
    frequency = std::clamp(frequency, 10., sampleRate * .495);

    // Normalized angular frequency in radians/sample.
    const double w = TwoPi * frequency / sampleRate;

    const double Cos = std::cos(w);
    const double Sin = std::sin(w);

    // Controls resonance/bandwidth around the cutoff frequency.
    const double Alpha = Sin / (2. * q);

    // Common denominator term used by all normalized coefficients.
    const double a0 = 1. + Alpha;

    double b0, b1, b2;

    if (type == PassType::LowPass)
    {
        // RBJ cookbook low-pass coefficients before normalization.
        b1 = (1. - Cos);
        b0 = b1 * .5;
        b2 = b0;
    }
    else
    {
        // RBJ cookbook high-pass coefficients before normalization.
        b0 =  (1. + Cos) * .5;
        b1 = -(1. + Cos);
        b2 = b0;
    }

    // Normalize feed-forward coefficients by a0 so the filter can be evaluated without dividing each sample.
    _B0 = b0 / a0;
    _B1 = b1 / a0;
    _B2 = b2 / a0;

    /// Normalize feedback coefficients. Direct Form II Transposed implementation expects these signs.
    _A1 = (-2 * Cos) / a0;
    _A2 = (1 - Alpha) / a0;

    return S_OK;
}

/// <summary>
/// Processes a sample.
/// </summary>
double biquad_t::Process(double amplitude) noexcept
{
    // Direct Form II Transposed structure. Compute current output using the input sample and first state.
    const double y = _B0 * amplitude + _Z1;

    // Update filter states for the next sample. z1 and z2 store the delayed internal accumulators, minimizing memory usage compared to Direct Form I.
    _Z1 = _B1 * amplitude - _A1 * y + _Z2;
    _Z2 = _B2 * amplitude - _A2 * y;

    return y;
}

/// <summary>
/// Configure this instance.
/// </summary>
HRESULT lr4_crossover_t::Configure(double frequency, double sampleRate) noexcept
{
    if ((frequency == _Frequency) && (sampleRate == _SampleRate))
        return S_FALSE;

    _LP1.Configure(biquad_t::PassType::LowPass, frequency, sampleRate);
    _LP2.Configure(biquad_t::PassType::LowPass, frequency, sampleRate);

    _HP1.Configure(biquad_t::PassType::HighPass, frequency, sampleRate);
    _HP2.Configure(biquad_t::PassType::HighPass, frequency, sampleRate);

    return S_OK;
}

/// <summary>
/// Splits the input signal into low and high.
/// </summary>
void lr4_crossover_t::Process(double amplitude, double & lowBand, double & highBand) noexcept
{
    lowBand  = _LP2.Process(_LP1.Process(amplitude)); // Two cascaded 2nd-order Butterworth low-pass filters implemented as a biquad.
    highBand = _HP2.Process(_HP1.Process(amplitude)); // Two cascaded 2nd-order Butterworth high-pass filters implemented as a biquad.
}

/// <summary>
/// Resets this instance.
/// </summary>
void lr4_crossover_t::Reset() noexcept
{
    _LP1.Reset();
    _LP2.Reset();

    _HP1.Reset();
    _HP2.Reset();
}

/// <summary>
/// Configures this instance.
/// </summary>
HRESULT crossover_filter_t::Configure(double loFreq, double hiFreq, double sampleRate) noexcept
{
    if ((loFreq >= hiFreq) || (sampleRate <= 0.))
        return E_INVALIDARG;

    if ((loFreq == _LoFreq) && (hiFreq == _HiFreq) && (sampleRate == _SampleRate))
        return S_FALSE;

    constexpr double TwoPi = 2. * std::numbers::pi_v<double>;

    _LoFreq     = loFreq;
    _HiFreq     = hiFreq;
    _SampleRate = sampleRate;

    _LowMid .Configure(loFreq, sampleRate);
    _MidHigh.Configure(hiFreq, sampleRate);

    _LoAlpha = 1. - std::exp(-TwoPi * _LoFreq / _SampleRate);
    _HiAlpha = 1. - std::exp(-TwoPi * _HiFreq / _SampleRate);

    Reset();

    return S_OK;
}

/// <summary>
/// Resets this instance.
/// </summary>
void crossover_filter_t::Reset() noexcept
{
    _LoLP = _HiLP = 0.;

    _LowMid.Reset();
    _MidHigh.Reset();
}

/// <summary>
/// Processes a sample.
/// </summary>
void crossover_filter_t::Process(double amplitude, double & lowBand, double & midBand, double & highBand) noexcept
{
    if (_Mode == Mode::LinkwitzRiley4)
    {
        // 24 dB/octave slopes, much better isolation, phase-aligned summation.
        double AboveLow = 0.;

        _LowMid.Process(amplitude, lowBand, AboveLow);
        _MidHigh.Process(AboveLow, midBand, highBand);
    }
    else
    if (_Mode == Mode::FirstOrder)
    {
        // 6 dB/octave slopes, large overlap due to the shallow slopes, substantial phase shift, broad mid band.
        _LoLP += _LoAlpha * (amplitude - _LoLP);
        _HiLP += _HiAlpha * (amplitude - _HiLP);

        lowBand  = _LoLP;
        highBand = amplitude - _HiLP;
        midBand  = _HiLP - _LoLP;
    }
    else
    {
        // Pass the signal unchanged.
        lowBand  = 0.;
        midBand  = amplitude;
        highBand = 0.;
    }
}

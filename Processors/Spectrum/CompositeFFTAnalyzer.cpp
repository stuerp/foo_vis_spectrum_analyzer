
/** $VER: CompositeFFTAnalyzer.cpp (2026.11.11) P. Stuer */

#include "pch.h"

#include "CompositeFFTAnalyzer.h"

#include <bit>
#include <limits>

#pragma hdrstop

namespace
{
    /// <summary>
    /// Rounds the requested transform size up to a power of two while leaving enough headroom to derive the two larger tier sizes without overflow.
    /// </summary>
    size_t GetBaseFFTSize(size_t requestedSize) noexcept
    {
        constexpr size_t MaximumPowerOfTwo = std::bit_floor(std::numeric_limits<size_t>::max());

        const size_t ClampedSize = std::clamp<size_t>(requestedSize, 2, MaximumPowerOfTwo / 4);

        return std::bit_ceil(ClampedSize);
    }

    /// <summary>
    /// Copies only the geometry required by the FFT band mapping.
    /// </summary>
    void CopyBands(const frequency_bands_t & src, frequency_bands_t & dst) noexcept
    {
        if (dst.size() != src.size())
            dst.resize(src.size());

        for (size_t i = 0; i < src.size(); ++i)
        {
            dst[i].Low    = src[i].Low;
            dst[i].Center = src[i].Center;
            dst[i].High   = src[i].High;
            dst[i].Magnitude = 0.;
        }
    }

    /// <summary>
    /// Turns a linear ramp into a smooth S-shaped curve.
    /// </summary>
    inline double Smoothstep(double t) noexcept
    {
        t = std::clamp(t, 0., 1.);

        return (t * t) * (3. - (2. * t));
    }

    /// <summary>
    /// Calculates how strongly a tier should contribute based on the number of its FFT bins covered by an output band.
    /// </summary>
    inline double GetResolutionWeight(double bandWidth, double binWidth) noexcept
    {
        constexpr double MinimumBinCount = 1.;
        constexpr double FullBinCount    = 2.;

        if ((bandWidth <= 0.) || (binWidth <= 0.))
            return 0.;

        const double BinCount = bandWidth / binWidth;
        const double t = (BinCount - MinimumBinCount) / (FullBinCount - MinimumBinCount);

        return Smoothstep(t);
    }
}

/// <summary>
/// Initializes an instance of the class.
/// </summary>
composite_fft_analyzer_t::composite_fft_analyzer_t(const state_t & state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction, const window_function_t & brownPucketteKernel) noexcept : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction)
{
    const size_t BaseFFTSize = GetBaseFFTSize(state._FFTSize);

    // Split the selected span into equal logarithmic ranges. These boundaries only select output bands; every FFT analyzes the same unfiltered input.
    _LowTier.Analyzer  = std::make_unique<fft_analyzer_t>(state, BaseFFTSize << 2, sampleRate, channelCount, channelSetup, windowFunction, brownPucketteKernel);
    _MidTier.Analyzer  = std::make_unique<fft_analyzer_t>(state, BaseFFTSize << 1, sampleRate, channelCount, channelSetup, windowFunction, brownPucketteKernel);
    _HighTier.Analyzer = std::make_unique<fft_analyzer_t>(state, BaseFFTSize,      sampleRate, channelCount, channelSetup, windowFunction, brownPucketteKernel);
}

/// <summary>
/// Analyzes the audio and returns the composited frequency bands. Cross-fade regions are derived from the FFT bin width. 
/// </summary>
bool composite_fft_analyzer_t::AnalyzeAudio(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept
{
    auto ProcessTier = [&](fft_tier_t & tier) noexcept -> bool
    {
        CopyBands(frequencyBands, tier.Bands);

        return tier.Analyzer->AnalyzeAudio(frames, frameCount, selectedChannels, tier.Bands);
    };

    if (!ProcessTier(_LowTier))
        return false;

    if (!ProcessTier(_MidTier))
        return false;

    if (!ProcessTier(_HighTier))
        return false;

    const double MidBinWidth  = (double) (_SampleRate) / (double) (_MidTier .Analyzer->GetFFTSize());
    const double HighBinWidth = (double) (_SampleRate) / (double) (_HighTier.Analyzer->GetFFTSize());

    // Merge the magnitudes according to the FFT resolution available for each output band.
    for (size_t i = 0; i < frequencyBands.size(); ++i)
    {
        const double BandWidth = std::max(0., frequencyBands[i].High - frequencyBands[i].Low);

        // Derived the tier’s resolution from its FFT bin width. A shorter tier starts taking over when the output band covers one of its bins and is fully active at two bins.
        const double MidWeight  = GetResolutionWeight(BandWidth, MidBinWidth);
        const double HighWeight = GetResolutionWeight(BandWidth, HighBinWidth);

        const double WeightLow  = 1. - MidWeight;
        const double WeightMid  = MidWeight * (1. - HighWeight);
        const double WeightHigh = HighWeight;

        const double MagnitudeLow  = _LowTier .Bands[i].Magnitude;
        const double MagnitudeMid  = _MidTier .Bands[i].Magnitude;
        const double MagnitudeHigh = _HighTier.Bands[i].Magnitude;

        frequencyBands[i].Magnitude = (WeightLow  * MagnitudeLow) + (WeightMid  * MagnitudeMid) + (WeightHigh * MagnitudeHigh);
    }

    return true;
}


/** $VER: CompositeFFTAnalyzer.cpp (2026.10.25) P. Stuer **/

#include "pch.h"

#include "CompositeFFTAnalyzer.h"

#include <array>
#include <bit>
#include <cmath>
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
}

/// <summary>
/// Initializes an instance of the class.
/// </summary>
composite_fft_analyzer_t::composite_fft_analyzer_t(const state_t & state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction, const window_function_t & brownPucketteKernel) noexcept : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction)
{
    const size_t BaseFFTSize = GetBaseFFTSize(state._FFTSize);

    const std::array<size_t, 3> TierFFTSize =
    {
        BaseFFTSize << 2,   // Low frequencies
        BaseFFTSize << 1,   // Middle frequencies
        BaseFFTSize         // High frequencies
    };

    // Split the selected span into equal logarithmic ranges. These boundaries only select output bands; every FFT analyzes the same unfiltered input.
    const double TierRatio = std::cbrt(state._HiFrequency / state._LoFrequency);

    const double LowCrossover  = state._LoFrequency  * TierRatio;
    const double HighCrossover = LowCrossover        * TierRatio;

    _Tiers.reserve(3);

    fft_tier_t LowTier;

    LowTier.Analyzer     = std::make_unique<fft_analyzer_t>(state, TierFFTSize[0], sampleRate, channelCount, channelSetup, windowFunction, brownPucketteKernel);
    LowTier.FFTSize      = TierFFTSize[0];
    LowTier.MinFrequency = state._LoFrequency;
    LowTier.MaxFrequency = LowCrossover;

    _Tiers.emplace_back(std::move(LowTier));

    fft_tier_t MidTier;

    MidTier.Analyzer     = std::make_unique<fft_analyzer_t>(state, TierFFTSize[1], sampleRate, channelCount, channelSetup, windowFunction, brownPucketteKernel);
    MidTier.FFTSize      = TierFFTSize[1];
    MidTier.MinFrequency = LowCrossover;
    MidTier.MaxFrequency = HighCrossover;

    _Tiers.emplace_back(std::move(MidTier));

    fft_tier_t HighTier;

    HighTier.Analyzer     = std::make_unique<fft_analyzer_t>(state, TierFFTSize[2], sampleRate, channelCount, channelSetup, windowFunction, brownPucketteKernel);
    HighTier.FFTSize      = TierFFTSize[2];
    HighTier.MinFrequency = HighCrossover;
    HighTier.MaxFrequency = state._HiFrequency;

    _Tiers.emplace_back(std::move(HighTier));

    // Temporal align the FFT windows. Align the center of every FFT window with the center of the longest FFT.
    const size_t LongestFFTSize = TierFFTSize[0];

    for (auto & Tier : _Tiers)
    {
        const size_t DelayFrameCount = (LongestFFTSize - Tier.FFTSize) / 2;

        Tier.DelayLine.assign(DelayFrameCount * channelCount, 0.);
    }
}

/// <summary>
/// Analyzes the audio and returns the composited frequency bands.
/// </summary>
bool composite_fft_analyzer_t::AnalyzeAudio(const audio_sample * frames, size_t frameCount, uint32_t channels, frequency_bands_t & frequencyBands) noexcept
{
    for (size_t TierIndex = 0; TierIndex < _Tiers.size(); ++TierIndex)
    {
        auto & Tier = _Tiers[TierIndex];

        CopyBandGeometry(frequencyBands, Tier.Bands);

        if (!AnalyzeTier(Tier, frames, frameCount, channels))
            return false;

        MaskTierBands(Tier, TierIndex + 1 == _Tiers.size());
    }

    MergeTiers(frequencyBands);

    return true;
}

/// <summary>
/// Analyzes a tier after applying its temporal-alignment delay.
/// </summary>
bool composite_fft_analyzer_t::AnalyzeTier(fft_tier_t & tier, const audio_sample * frames, size_t frameCount, uint32_t channels) noexcept
{
    if (tier.DelayLine.empty())
        return tier.Analyzer->AnalyzeAudio(frames, frameCount, channels, tier.Bands);

    const size_t SampleCount = frameCount * _ChannelCount;

    tier.DelayedFrames.resize(SampleCount);

    for (size_t i = 0; i < SampleCount; ++i)
    {
        tier.DelayedFrames[i] = tier.DelayLine[tier.DelayPosition];
        tier.DelayLine[tier.DelayPosition] = frames[i];

        if (++tier.DelayPosition == tier.DelayLine.size())
            tier.DelayPosition = 0;
    }

    return tier.Analyzer->AnalyzeAudio(tier.DelayedFrames.data(), frameCount, channels, tier.Bands);
}

/// <summary>
/// Copies only the geometry required by the FFT band mapping.
/// </summary>
void composite_fft_analyzer_t::CopyBandGeometry(const frequency_bands_t & source, frequency_bands_t & destination) noexcept
{
    destination.resize(source.size());

    for (size_t i = 0; i < source.size(); ++i)
    {
        destination[i].Low    = source[i].Low;
        destination[i].Center = source[i].Center;
        destination[i].High   = source[i].High;
        destination[i].Magnitude = 0.;
    }
}

/// <summary>
/// Removes all mapped output bands that do not belong to this tier. Tier ranges are half-open, except for the final tier, so no band is duplicated.
/// </summary>
void composite_fft_analyzer_t::MaskTierBands(fft_tier_t & tier, bool isLastTier) noexcept
{
    for (auto & Band : tier.Bands)
    {
        const bool IsInTier = (Band.Center >= tier.MinFrequency) && (isLastTier ? (Band.Center <= tier.MaxFrequency) : (Band.Center < tier.MaxFrequency));

        if (!IsInTier)
            Band.Magnitude = 0.;
    }
}

/// <summary>
/// Copies the already-masked result from the single tier assigned to each band.
/// No magnitudes from different FFT sizes are blended or summed.
/// </summary>
void composite_fft_analyzer_t::MergeTiers(frequency_bands_t & frequencyBands) const noexcept
{
    size_t TierIndex = 0;

    for (size_t i = 0; i < frequencyBands.size(); ++i)
    {
        auto & Band = frequencyBands[i];

        while ((TierIndex + 1 < _Tiers.size()) && (Band.Center >= _Tiers[TierIndex].MaxFrequency))
            ++TierIndex;

        const auto & Tier = _Tiers[TierIndex];

        const bool IsLastTier = TierIndex + 1 == _Tiers.size();
        const bool IsInTier   = (Band.Center >= Tier.MinFrequency) && (IsLastTier ? (Band.Center <= Tier.MaxFrequency) : (Band.Center < Tier.MaxFrequency));

        Band.Magnitude = IsInTier && (i < Tier.Bands.size()) ? Tier.Bands[i].Magnitude : 0.;
    }
}


/** $VER: CompositeFFTAnalyzer.h (2026.10.25) P. Stuer - Composite multi-tier FFT analyzer. **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <memory>
#include <vector>

#include "Analyzer.h"
#include "FrequencyBand.h"
#include "FFTAnalyzer.h"

/// <summary>
/// Describes one FFT tier and its frequency range.
/// </summary>
struct fft_tier_t
{
    std::unique_ptr<fft_analyzer_t> Analyzer;

    size_t FFTSize = 0;
    double MinFrequency = 0.;
    double MaxFrequency = 0.;

    frequency_bands_t Bands;

    // Delays shorter transforms so all FFT windows have the same center time.
    std::vector<audio_sample> DelayLine;
    std::vector<audio_sample> DelayedFrames;
    size_t DelayPosition = 0;
};

/// <summary>
/// Implements a composite FFT analyzer consisting of three time-aligned FFT
/// tiers. Every output band is supplied by exactly one tier.
/// </summary>
class composite_fft_analyzer_t final : public analyzer_t
{
public:
    composite_fft_analyzer_t() = delete;

    composite_fft_analyzer_t(const composite_fft_analyzer_t &) = delete;
    composite_fft_analyzer_t(composite_fft_analyzer_t &&) = delete;

    composite_fft_analyzer_t & operator=(const composite_fft_analyzer_t &) = delete;
    composite_fft_analyzer_t & operator=(composite_fft_analyzer_t &&) = delete;

    virtual ~composite_fft_analyzer_t() = default;

    composite_fft_analyzer_t(const state_t & state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction, const window_function_t & brownPucketteKernel) noexcept;

    bool AnalyzeAudio(const audio_sample * frames, size_t frameCount, uint32_t channels, frequency_bands_t & frequencyBands) noexcept override final;

private:
    static void CopyBandGeometry(const frequency_bands_t & source, frequency_bands_t & destination) noexcept;
    static void MaskTierBands(fft_tier_t & tier, bool isLastTier) noexcept;

    bool AnalyzeTier(fft_tier_t & tier, const audio_sample * frames, size_t frameCount, uint32_t channels) noexcept;
    void MergeTiers(frequency_bands_t & frequencyBands) const noexcept;

private:
    std::vector<fft_tier_t> _Tiers;
};


/** $VER: CompositeFFTAnalyzer.h (2026.10.31) P. Stuer - Composite multi-tier FFT analyzer. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <array>
#include <memory>

#include "Analyzer.h"
#include "FrequencyBand.h"
#include "FFTAnalyzer.h"

/// <summary>
/// Describes one FFT tier and its frequency range.
/// </summary>
struct fft_tier_t
{
    std::unique_ptr<fft_analyzer_t> Analyzer;
    frequency_bands_t Bands;
};

/// <summary>
/// Implements a composite FFT analyzer consisting of three time-aligned FFT tiers.
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

    bool AnalyzeAudio(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept override final;

private:
    fft_tier_t _LowTier;
    fft_tier_t _MidTier;
    fft_tier_t _HighTier;
};


/** $VER: SpectrumProcessor.h (2026.10.28) P. Stuer - Implements an audio processor for the spectrum visualizations. **/

#pragma once

#include "AudioProcessor.h"
#include "State.h"

#include "Analyzer.h"
#include "FrequencyBand.h"
#include "WindowFunctions.h"

#include <sdk/audio_chunk.h>

namespace Spectrum
{

class processor_t final : public audio_processor_t
{
public:
    processor_t(state_t & state) : audio_processor_t(state) { }

    processor_t(const processor_t &) = delete;
    processor_t(processor_t &&) = delete;

    processor_t & operator=(const processor_t &) = delete;
    processor_t & operator=(processor_t &&) = delete;

    virtual ~processor_t() = default;

public:
    void Configure(const graph_options_t * options) noexcept override final;
    void Reset() noexcept override final;
    void Process(const audio_chunk & chunk) noexcept override final;
    void UpdatePeaks(bool isStopped) noexcept override final;
    void ResetPeaks() noexcept override final;

public:
    static double GetAcousticWeight(double x, WeightingType weightingType, double weightAmount) noexcept;

private:
    void CreateAnalyzer() noexcept;

    void GenerateLinearFrequencyBands();
    void GenerateOctaveFrequencyBands();
    void GenerateAveePlayerFrequencyBands();
    void GenerateMelFrequencyBands();
    void GenerateDecadeAlignedFrequencyBands();

    void ApplyAcousticWeighting() noexcept;

    double GetWeight(double frequency) const noexcept;

    static inline double GetFrequencyTilt(double x, double amount, double offset) noexcept;
    static inline double Equalize(double x, double amount, double depth, double offset) noexcept;

    void Normalize() noexcept;
    void NormalizeWithAverageSmoothing(double factor) noexcept;
    void NormalizeWithPeakSmoothing(double factor) noexcept;

public:
    frequency_bands_t FrequencyBands;
    double NyquistFrequency = 0.;

    std::unique_ptr<window_function_t> WindowFunction;

private:
    std::unique_ptr<analyzer_t> _Analyzer;

    std::unique_ptr<window_function_t> _BrownPucketteKernel;
};

}

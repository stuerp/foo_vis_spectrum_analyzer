
/** $VER: Analysis.h (2024.02.17) P. Stuer **/

#pragma once

#include "framework.h"

#include "State.h"

#include "WindowFunctions.h"

#include "FFTAnalyzer.h"
#include "CQTAnalyzer.h"
#include "SWIFTAnalyzer.h"

#include "FrequencyBand.h"

/// <summary>
/// Represents the analysis of the sample data.
/// </summary>
class Analysis
{
public:
    Analysis() { };

    Analysis(const Analysis &) = delete;
    Analysis & operator=(const Analysis &) = delete;
    Analysis(Analysis &&) = delete;
    Analysis & operator=(Analysis &&) = delete;

    virtual ~Analysis() { Reset(); };

    void Initialize(const State * state, uint32_t channels) noexcept;
    void Process(const audio_chunk & chunk) noexcept;
    void UpdatePeakIndicators() noexcept;

    void Reset();

private:
    void GetAnalyzer(const audio_chunk & chunk) noexcept;

    void ApplyAcousticWeighting();
    double GetWeight(double x) const noexcept;

    void ApplyAverageSmoothing(double factor) noexcept;
    void ApplyPeakSmoothing(double factor) noexcept;

    void GenerateLinearFrequencyBands(const State * state);
    void GenerateOctaveFrequencyBands(const State * state);
    void GenerateAveePlayerFrequencyBands(const State * state);

    static double ScaleF(double x, ScalingFunction function, double factor);
    static double DeScaleF(double x, ScalingFunction function, double factor);

public:
    const State * _State;
    uint32_t _Channels;

    uint32_t _SampleRate;

    const WindowFunction * _WindowFunction;
    const WindowFunction * _BrownPucketteKernel;

    FFTAnalyzer * _FFTAnalyzer;
    CQTAnalyzer * _CQTAnalyzer;
    SWIFTAnalyzer * _SWIFTAnalyzer;

    FrequencyBands _FrequencyBands;
};

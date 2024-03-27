
/** $VER: Analysis.h (2024.03.27) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "State.h"

#include "WindowFunctions.h"

#include "FFTAnalyzer.h"
#include "CQTAnalyzer.h"
#include "SWIFTAnalyzer.h"
#include "AnalogStyleAnalyzer.h"

#include "FrequencyBand.h"

/// <summary>
/// Represents a meter value of a channel.
/// </summary>
struct MeterValue
{
    audio_sample Peak;
    audio_sample RMS;
};

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

    void Initialize(const State * state, const GraphSettings * settings) noexcept;
    void Process(const audio_chunk & chunk) noexcept;
    void UpdatePeakIndicators() noexcept;

    void Reset();

private:
    void GenerateLinearFrequencyBands();
    void GenerateOctaveFrequencyBands();
    void GenerateAveePlayerFrequencyBands();

    bool GetMeterValues(const audio_chunk & chunk) noexcept;

    void GetAnalyzer(const audio_chunk & chunk) noexcept;

    void ApplyAcousticWeighting();
    double GetWeight(double x) const noexcept;

    void Normalize() noexcept;
    void NormalizeWithAverageSmoothing(double factor) noexcept;
    void NormalizeWithPeakSmoothing(double factor) noexcept;

public:
    const State * _ThreadState;
    const GraphSettings * _GraphSettings;

    uint32_t _SampleRate;
    std::vector<MeterValue> _MeterValues;

    const WindowFunction * _WindowFunction;
    const WindowFunction * _BrownPucketteKernel;

    FFTAnalyzer * _FFTAnalyzer;
    CQTAnalyzer * _CQTAnalyzer;
    SWIFTAnalyzer * _SWIFTAnalyzer;
    AnalogStyleAnalyzer * _AnalogStyleAnalyzer;

    FrequencyBands _FrequencyBands;
};

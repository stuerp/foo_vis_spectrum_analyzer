
/** $VER: SWIFTAnalyzer.h (2024.03.02) P. Stuer - Based on TF3RDL Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

#pragma once

#include "framework.h"

#include "Analyzer.h"
#include "FrequencyBand.h"

#include <vector>

/// <summary>
/// Implements a Sliding Windowed Infinite Fourier Transform (SWIFT) analyzer.
/// </summary>
class SWIFTAnalyzer : public Analyzer
{
public:
    SWIFTAnalyzer() = delete;

    SWIFTAnalyzer(const SWIFTAnalyzer &) = delete;
    SWIFTAnalyzer & operator=(const SWIFTAnalyzer &) = delete;
    SWIFTAnalyzer(SWIFTAnalyzer &&) = delete;
    SWIFTAnalyzer & operator=(SWIFTAnalyzer &&) = delete;

    virtual ~SWIFTAnalyzer() { }

    SWIFTAnalyzer(const State * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup);

    bool Initialize(const vector<FrequencyBand> & frequencyBands);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept;

private:
    struct Value
    {
        double x;
        double y;
    };

    struct Coef
    {
        double rX;
        double rY;
        double Decay;
        Value Values[MaxFilterBankOrder];
    };

    std::vector<Coef> _Coefs;
};

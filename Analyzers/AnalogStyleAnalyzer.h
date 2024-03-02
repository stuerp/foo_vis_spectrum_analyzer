
/** $VER: AnalogStyleAnalyzer.h (2024.03.02) P. Stuer **/

#pragma once

#include "framework.h"

#include "Analyzer.h"
#include "FrequencyBand.h"

/// <summary>
/// Implements an Analog-style spectrum analyzer.
/// </summary>
class AnalogStyleAnalyzer : public Analyzer
{
public:
    AnalogStyleAnalyzer() = delete;

    AnalogStyleAnalyzer(const AnalogStyleAnalyzer &) = delete;
    AnalogStyleAnalyzer & operator=(const AnalogStyleAnalyzer &) = delete;
    AnalogStyleAnalyzer(AnalogStyleAnalyzer &&) = delete;
    AnalogStyleAnalyzer & operator=(AnalogStyleAnalyzer &&) = delete;

    virtual ~AnalogStyleAnalyzer() { }

    AnalogStyleAnalyzer(const State * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction);

    bool Initialize(const vector<FrequencyBand> & frequencyBands);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept;

private:
    struct Coef
    {
        double a0;
        double a1;
        double a2;
        double b1;
        double b2;

        double z1[MaxFilterBankOrder];
        double z2[MaxFilterBankOrder];

        double Out[MaxFilterBankOrder];
    };

    std::vector<Coef> _Coefs;
};


/** $VER: SWIFTAnalyzer.h (2024.02.11) P. Stuer - Based on TF3RDL Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "TransformProvider.h"
#include "FrequencyBand.h"

#include <vector>

/// <summary>
/// Implements a Sliding Windowed Infinite Fourier Transform (SWIFT) analyzer.
/// </summary>
class SWIFTAnalyzer : public TransformProvider
{
public:
    SWIFTAnalyzer() = delete;

    SWIFTAnalyzer(const SWIFTAnalyzer &) = delete;
    SWIFTAnalyzer & operator=(const SWIFTAnalyzer &) = delete;
    SWIFTAnalyzer(SWIFTAnalyzer &&) = delete;
    SWIFTAnalyzer & operator=(SWIFTAnalyzer &&) = delete;

    virtual ~SWIFTAnalyzer() { }

    SWIFTAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const Configuration * configuration);

    bool Initialize(const vector<FrequencyBand> & frequencyBands);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & frequencyBands);

private:
    const Configuration * _Configuration;

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
        std::vector<Value> Values;
    };

    std::vector<Coef> _Coefs;
};

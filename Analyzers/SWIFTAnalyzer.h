
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

    SWIFTAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction);

    bool Initialize(const vector<FrequencyBand> & frequencyBands, size_t order, double timeRes, double bandwidth);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channelMask, vector<FrequencyBand> & frequencyBands);

private:
    struct Point
    {
        double x;
        double y;
    };

    struct Coef
    {
        double rX;
        double rY;
        double decay;
        std::vector<Point> coeffs;
    };

    std::vector<Coef> _Coefs;
    double * _spectrumData;
};

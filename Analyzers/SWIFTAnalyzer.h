
/** $VER: SWIFTAnalyzer.h (2024.03.09) P. Stuer - Based on TF3RDL Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Analyzer.h"
#include "FrequencyBand.h"

#include <vector>

/// <summary>
/// Implements a Sliding Windowed Infinite Fourier Transform (SWIFT) analyzer.
/// </summary>
#pragma warning(disable: 4820)
class SWIFTAnalyzer : public analyzer_t
{
public:
    SWIFTAnalyzer() = delete;

    SWIFTAnalyzer(const SWIFTAnalyzer &) = delete;
    SWIFTAnalyzer & operator=(const SWIFTAnalyzer &) = delete;
    SWIFTAnalyzer(SWIFTAnalyzer &&) = delete;
    SWIFTAnalyzer & operator=(SWIFTAnalyzer &&) = delete;

    virtual ~SWIFTAnalyzer() { }

    SWIFTAnalyzer(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup);

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

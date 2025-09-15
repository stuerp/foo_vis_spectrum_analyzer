
/** $VER: SWIFTAnalyzer.h (2025.09.15) P. Stuer - Based on TF3RDL Sliding Windowed Infinite Fourier Transform (SWIFT), https://codepen.io/TF3RDL/pen/JjBzjeY **/

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
class swift_analyzer_t : public analyzer_t
{
public:
    swift_analyzer_t() = delete;

    swift_analyzer_t(const swift_analyzer_t &) = delete;
    swift_analyzer_t & operator=(const swift_analyzer_t &) = delete;
    swift_analyzer_t(swift_analyzer_t &&) = delete;
    swift_analyzer_t & operator=(swift_analyzer_t &&) = delete;

    virtual ~swift_analyzer_t() { }

    swift_analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup);

    bool Initialize(const FrequencyBands & frequencyBands) noexcept;
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept;

private:
    struct swift_value_t
    {
        double x;
        double y;
    };

    struct swift_coef_t
    {
        swift_coef_t() noexcept { };
        swift_coef_t(double rx, double ry, double decay) noexcept : rX(rx), rY(ry), Decay(decay), Values() { }

        double rX;
        double rY;
        double Decay;
        swift_value_t Values[MaxFilterBankOrder];
    };

    std::vector<swift_coef_t> _Coefs;
};

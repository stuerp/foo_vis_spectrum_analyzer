
/** $VER: AnalogStyleAnalyzer.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Analyzer.h"
#include "FrequencyBand.h"

/// <summary>
/// Implements an Analog-style spectrum analyzer.
/// </summary>
#pragma warning(disable: 4820)
class analog_style_analyzer_t : public analyzer_t
{
public:
    analog_style_analyzer_t() = delete;

    analog_style_analyzer_t(const analog_style_analyzer_t &) = delete;
    analog_style_analyzer_t & operator=(const analog_style_analyzer_t &) = delete;
    analog_style_analyzer_t(analog_style_analyzer_t &&) = delete;
    analog_style_analyzer_t & operator=(analog_style_analyzer_t &&) = delete;

    virtual ~analog_style_analyzer_t() { }

    analog_style_analyzer_t(const state_t * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction);

    bool Initialize(const vector<frequency_band_t> & frequencyBands);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, frequency_bands_t & frequencyBands) noexcept;

private:
    struct coef_t
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

    std::vector<coef_t> _Coefs;
};

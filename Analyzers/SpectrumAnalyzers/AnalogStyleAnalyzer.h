
/** $VER: AnalogStyleAnalyzer.h (2026.10.24) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Analyzer.h"
#include "FrequencyBand.h"

#include <vector>

/// <summary>
/// Implements an Analog-style spectrum analyzer.
/// </summary>
class analog_style_analyzer_t final : public analyzer_t
{
public:
    analog_style_analyzer_t() = delete;

    analog_style_analyzer_t(const analog_style_analyzer_t &) = delete;
    analog_style_analyzer_t(analog_style_analyzer_t &&) = delete;

    analog_style_analyzer_t & operator=(const analog_style_analyzer_t &) = delete;
    analog_style_analyzer_t & operator=(analog_style_analyzer_t &&) = delete;

    virtual ~analog_style_analyzer_t() = default;

    analog_style_analyzer_t(const state_t & state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction);

    bool Initialize(const vector<frequency_band_t> & frequencyBands);
    bool AnalyzeAudio(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept override final;

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

    static constexpr double Pi    = std::numbers::pi;
    static constexpr double TwoPi = 2. * Pi;
    static constexpr double Pi2   = Pi * 0.5;
};

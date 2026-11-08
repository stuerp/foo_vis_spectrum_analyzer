
/** $VER: SWIFTAnalyzer.h (2026.10.24) P. Stuer - Implements a Sliding Windowed Infinite Fourier Transform (SWIFT) analyzer. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Analyzer.h"
#include "FrequencyBand.h"

#include <vector>

class swift_analyzer_t final : public analyzer_t
{
public:
    swift_analyzer_t() = delete;

    swift_analyzer_t(const swift_analyzer_t &) = delete;
    swift_analyzer_t(swift_analyzer_t &&) = delete;

    swift_analyzer_t & operator=(const swift_analyzer_t &) = delete;
    swift_analyzer_t & operator=(swift_analyzer_t &&) = delete;

    virtual ~swift_analyzer_t() = default;

    swift_analyzer_t(const state_t & state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup);

    bool Initialize(const frequency_bands_t & frequencyBands) noexcept;
    bool AnalyzeAudio(const audio_sample * frames , size_t frameCount, uint32_t channels, frequency_bands_t & frequencyBands) noexcept override final;

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

    static constexpr double Pi    = std::numbers::pi;
    static constexpr double TwoPi = 2. * Pi;
    static constexpr double Pi2   = Pi * 0.5;
};

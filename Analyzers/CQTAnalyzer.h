
/** $VER: CQTAnalyzer.h (2026.10.24) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Analyzer.h"
#include "FrequencyBand.h"

#include <vector>

/// <summary>
/// Implements a Constant-Q Transform analyzer.
/// </summary>
class cqt_analyzer_t : public analyzer_t
{
public:
    cqt_analyzer_t() = delete;

    cqt_analyzer_t(const cqt_analyzer_t &) = delete;
    cqt_analyzer_t(cqt_analyzer_t &&) = delete;

    cqt_analyzer_t & operator=(const cqt_analyzer_t &) = delete;
    cqt_analyzer_t & operator=(cqt_analyzer_t &&) = delete;

    virtual ~cqt_analyzer_t() = default;

    cqt_analyzer_t(const state_t & state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction);

    bool AnalyzeAudio(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept;

private:
    std::vector<audio_sample> _MonoSamples;

    static constexpr double Pi    = std::numbers::pi;
    static constexpr double TwoPi = 2. * Pi;
    static constexpr double Pi2   = Pi * 0.5;
};

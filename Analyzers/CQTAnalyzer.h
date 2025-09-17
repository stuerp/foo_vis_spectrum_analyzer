
/** $VER: CQTAnalyzer.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Analyzer.h"
#include "FrequencyBand.h"

/// <summary>
/// Implements a Constant-Q Transform analyzer.
/// </summary>
#pragma warning(disable: 4820)
class cqt_analyzer_t : public analyzer_t
{
public:
    cqt_analyzer_t() = delete;

    cqt_analyzer_t(const cqt_analyzer_t &) = delete;
    cqt_analyzer_t & operator=(const cqt_analyzer_t &) = delete;
    cqt_analyzer_t(cqt_analyzer_t &&) = delete;
    cqt_analyzer_t & operator=(cqt_analyzer_t &&) = delete;

    virtual ~cqt_analyzer_t() { }

    cqt_analyzer_t(const state_t * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, frequency_bands_t & frequencyBands) noexcept;
};

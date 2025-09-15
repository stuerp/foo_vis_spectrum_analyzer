
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
class CQTAnalyzer : public analyzer_t
{
public:
    CQTAnalyzer() = delete;

    CQTAnalyzer(const CQTAnalyzer &) = delete;
    CQTAnalyzer & operator=(const CQTAnalyzer &) = delete;
    CQTAnalyzer(CQTAnalyzer &&) = delete;
    CQTAnalyzer & operator=(CQTAnalyzer &&) = delete;

    virtual ~CQTAnalyzer() { }

    CQTAnalyzer(const state_t * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept;
};

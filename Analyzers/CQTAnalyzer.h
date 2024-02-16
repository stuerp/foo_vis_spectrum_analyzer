
/** $VER: CQTAnalyzer.h (2024.02.16) P. Stuer **/

#pragma once

#include "framework.h"

#include "State.h"
#include "Analyzer.h"
#include "Analysis.h"

#include <vector>

/// <summary>
/// Implements a Constant-Q Transform analyzer.
/// </summary>
class CQTAnalyzer : public Analyzer
{
public:
    CQTAnalyzer() = delete;

    CQTAnalyzer(const CQTAnalyzer &) = delete;
    CQTAnalyzer & operator=(const CQTAnalyzer &) = delete;
    CQTAnalyzer(CQTAnalyzer &&) = delete;
    CQTAnalyzer & operator=(CQTAnalyzer &&) = delete;

    virtual ~CQTAnalyzer() { }

    CQTAnalyzer(const State * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, Analyses & analyses) const;
};

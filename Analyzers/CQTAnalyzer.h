
/** $VER: CQTAnalyzer.h (2024.02.12) P. Stuer **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "Analyzer.h"
#include "FrequencyBand.h"

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

    CQTAnalyzer(const Configuration * configuration, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & frequencyBands) const;
};

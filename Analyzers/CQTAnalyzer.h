
/** $VER: CQTAnalyzer.h (2024.02.12) P. Stuer **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "TransformProvider.h"
#include "FrequencyBand.h"

#include <vector>

/// <summary>
/// Implements a Constant-Q Transform analyzer.
/// </summary>
class CQTAnalyzer : public TransformProvider
{
public:
    CQTAnalyzer() = delete;

    CQTAnalyzer(const CQTAnalyzer &) = delete;
    CQTAnalyzer & operator=(const CQTAnalyzer &) = delete;
    CQTAnalyzer(CQTAnalyzer &&) = delete;
    CQTAnalyzer & operator=(CQTAnalyzer &&) = delete;

    virtual ~CQTAnalyzer() { }

    CQTAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, double bandwidthOffset, double alignment, double downSample, const Configuration * configuration);
    bool AnalyzeSamples(const audio_sample * sampleData, size_t sampleCount, vector<FrequencyBand> & frequencyBands) const;

private:
    const Configuration * _Configuration;

    double _BandwidthOffset;
    double _Alignment;
    double _DownSample;
};

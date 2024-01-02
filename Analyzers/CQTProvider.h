
/** $VER: CQTProvider.h (2024.01.02) P. Stuer **/

#pragma once

#include "framework.h"

#include <vector>
#include <complex>

using namespace std;

#include "TransformProvider.h"
#include "FrequencyBand.h"

/// <summary>
/// Implements a Constant-Q Transform provider.
/// </summary>
class CQTProvider : public TransformProvider
{
public:
    CQTProvider() = delete;

    CQTProvider(const CQTProvider &) = delete;
    CQTProvider & operator=(const CQTProvider &) = delete;
    CQTProvider(CQTProvider &&) = delete;
    CQTProvider & operator=(CQTProvider &&) = delete;

    virtual ~CQTProvider() { }

    CQTProvider(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, double bandwidthOffset, double alignment, double downSample);
    bool GetFrequencyBands(const audio_sample * sampleData, size_t sampleCount, uint32_t channelMask, vector<FrequencyBand> & frequencyBands) const;

private:
    double _BandwidthOffset;
    double _Alignment;
    double _DownSample;
};

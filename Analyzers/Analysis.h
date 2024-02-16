
/** $VER: Analysis.h (2024.02.16) P. Stuer **/

#pragma once

#include "framework.h"

#include "State.h"
#include "FrequencyBand.h"

#include <vector>

/// <summary>
/// Represents the analysis of the sample data.
/// </summary>
class Analysis
{
public:
    Analysis(uint32_t channels) : _Channels(channels) { };

    Analysis(const Analysis &) = delete;
    Analysis & operator=(const Analysis &) = delete;
    Analysis(Analysis &&) = delete;
    Analysis & operator=(Analysis &&) = delete;

    virtual ~Analysis() { };

    void Initialize(const State & state) noexcept;

private:
    void GenerateLinearFrequencyBands(const State & state);
    void GenerateOctaveFrequencyBands(const State & state);
    void GenerateAveePlayerFrequencyBands(const State & state);

    static double ScaleF(double x, ScalingFunction function, double factor);
    static double DeScaleF(double x, ScalingFunction function, double factor);

public:
    FrequencyBands _FrequencyBands;
    uint32_t _Channels;
};

typedef std::vector<Analysis *> Analyses;

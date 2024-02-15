
/** $VER: Analysis.h (2024.02.15) P. Stuer **/

#pragma once

#include "framework.h"

#include "State.h"
#include "WindowFunctions.h"

#include "FFTAnalyzer.h"
#include "CQTAnalyzer.h"
#include "SWIFTAnalyzer.h"

using namespace std;

#include <vector>

/// <summary>
/// Aggregates the analyzer, window function and frequency bands.
/// </summary>
class Analysis
{
public:
    Analysis(State & state) : _State(state) { };

    Analysis(const Analysis &) = delete;
    Analysis & operator=(const Analysis &) = delete;
    Analysis(Analysis &&) = delete;
    Analysis & operator=(Analysis &&) = delete;

    virtual ~Analysis() { }

    void Initialize(const audio_chunk & chunk) noexcept;

public:
    State & _State;

    const WindowFunction * _WindowFunction;
    const WindowFunction * _BrownPucketteKernel;

    FFTAnalyzer * _FFTAnalyzer;
    CQTAnalyzer * _CQTAnalyzer;
    SWIFTAnalyzer * _SWIFTAnalyzer;

    std::vector<FrequencyBand> _FrequencyBands;
};

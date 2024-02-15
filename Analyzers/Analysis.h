
/** $VER: Analysis.h (2024.02.15) P. Stuer **/

#pragma once

#include "framework.h"

#include "FFTAnalyzer.h"
#include "CQTAnalyzer.h"
#include "SWIFTAnalyzer.h"

/// <summary>
/// Aggregates the analyzer and its output.
/// </summary>
class Analysis
{
public:
    Analysis() = delete;

    Analysis(const Analysis &) = delete;
    Analysis & operator=(const Analysis &) = delete;
    Analysis(Analysis &&) = delete;
    Analysis & operator=(Analysis &&) = delete;

    virtual ~Analysis() { };

public:
    const WindowFunction * _WindowFunction;
    const WindowFunction * _BrownPucketteKernel;

    FFTAnalyzer * _FFTAnalyzer;
    CQTAnalyzer * _CQTAnalyzer;
    SWIFTAnalyzer * _SWIFTAnalyzer;

    std::vector<FrequencyBand> _FrequencyBands;
};


/** $VER: Analysis.h (2024.04.24) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "State.h"

#include "WindowFunctions.h"

#include "FFTAnalyzer.h"
#include "CQTAnalyzer.h"
#include "SWIFTAnalyzer.h"
#include "AnalogStyleAnalyzer.h"

#include "FrequencyBand.h"

/// <summary>
/// Represents the values of a gauge.
/// </summary>
struct GaugeValue
{
    GaugeValue(const WCHAR * name = L"", double peak = -std::numeric_limits<double>::infinity(), double holdTime = 5.) : Name(name), Peak(peak), RMS(), HoldTime(holdTime)
    {
        Reset();
    }

    void Reset() noexcept
    {
        RMSTotal = 0.;
    }

    std::wstring Name;

    double RMSTotal;        // RMS value for the current RMS window.

    double Peak;            // in dBFS
    double PeakRender;      // 0.0 .. 1.0, Normalized and smoothed value used for rendering

    double RMS;             // in dBFS
    double RMSRender;       // 0.0 .. 1.0, Normalized and smoothed value used for rendering

    double MaxPeakRender;   // 0.0 .. 1.0, Normalized and smoothed value used for rendering
    double HoldTime;        // Time to hold the current max value.
    double DecaySpeed;      // Speed at which the current max value decays.
    double Opacity;         // 0.0 .. 1.0
};

/// <summary>
/// Represents the analysis of the sample data.
/// </summary>
class Analysis
{
public:
    Analysis() { };

    Analysis(const Analysis &) = delete;
    Analysis & operator=(const Analysis &) = delete;
    Analysis(Analysis &&) = delete;
    Analysis & operator=(Analysis &&) = delete;

    virtual ~Analysis() { Reset(); };

    void Initialize(const State * state, const GraphSettings * settings) noexcept;
    void Process(const audio_chunk & chunk) noexcept;
    void UpdatePeakValues() noexcept;

    void Reset();

private:
    // Spectrum
    void GenerateLinearFrequencyBands();
    void GenerateOctaveFrequencyBands();
    void GenerateAveePlayerFrequencyBands();

    void GetAnalyzer(const audio_chunk & chunk) noexcept;
    void GetGaugeValues(const audio_chunk & chunk) noexcept;

    void ApplyAcousticWeighting();
    double GetWeight(double x) const noexcept;

    void Normalize() noexcept;
    void NormalizeWithAverageSmoothing(double factor) noexcept;
    void NormalizeWithPeakSmoothing(double factor) noexcept;

    // Peak Meter
    double NormalizeValue(double amplitude) const noexcept
    {
        return std::clamp(Map(amplitude, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0., 1.), 0., 1.);
    }

    double SmoothValue(double value, double smoothedValue) const noexcept
    {
        switch (_State->_SmoothingMethod)
        {
            default:

            case SmoothingMethod::None:
                return value;

            case SmoothingMethod::Average:
                return std::clamp((smoothedValue * _State->_SmoothingFactor) + (value * (1. - _State->_SmoothingFactor)), 0., 1.);

            case SmoothingMethod::Peak:
                return std::clamp(std::max(smoothedValue * _State->_SmoothingFactor, value), 0., 1.);
        }
    }

public:
    const State * _State;
    const GraphSettings * _GraphSettings;

    uint32_t _SampleRate;
    double _NyquistFrequency;
    std::vector<GaugeValue> _GaugeValues;

    const WindowFunction * _WindowFunction;
    const WindowFunction * _BrownPucketteKernel;

    FFTAnalyzer * _FFTAnalyzer;
    CQTAnalyzer * _CQTAnalyzer;
    SWIFTAnalyzer * _SWIFTAnalyzer;
    AnalogStyleAnalyzer * _AnalogStyleAnalyzer;

    FrequencyBands _FrequencyBands;

    // Peak Meter
    double _RMSTimeElapsed; // Elapsed time in the current RMS window (in seconds).
    size_t _RMSSampleCount; // Number of samples used in the current RMS window.

    // Balance Meter
    double _Left;           // in dBFS
    double _Right;          // in dBFS

    double _Mid;            // in dBFS
    double _Side;           // in dBFS

    double _Balance;        // -1.0 .. 1.0
    double _Phase;          // -1.0 .. 1.0

private:
    const double Amax = M_SQRT1_2;
    const double dBCorrection = -20. * ::log10(Amax); // 3.01;
};

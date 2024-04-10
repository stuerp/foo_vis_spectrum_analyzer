
/** $VER: Analysis.h (2024.04.10) P. Stuer **/

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
/// Represents a meter value of a channel.
/// </summary>
struct MeterValue
{
    MeterValue(const WCHAR * name = L"", double peak = 0., double rms = 0., double holdTime = 5., double decaySpeed = 0.) : Name(name), Peak(peak), RMS(rms), HoldTime(holdTime), DecaySpeed(decaySpeed) { }

    std::wstring Name;

    double Peak;            // dB
    double NormalizedPeak;  // 0.0 .. 1.0
    double SmoothedPeak;    // 0.0 .. 1.0

    double RMS;             // dB
    double NormalizedRMS;   // 0.0 .. 1.0
    double SmoothedRMS;     // 0.0 .. 1.0

    double MaxSmoothedPeak; // 0.0 .. 1.0, The value of the maximum peak indicator
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
    void GenerateLinearFrequencyBands();
    void GenerateOctaveFrequencyBands();
    void GenerateAveePlayerFrequencyBands();

    bool GetMeterValues(const audio_chunk & chunk) noexcept;

    void GetAnalyzer(const audio_chunk & chunk) noexcept;

    void ApplyAcousticWeighting();
    double GetWeight(double x) const noexcept;

    void Normalize() noexcept;
    void NormalizeWithAverageSmoothing(double factor) noexcept;
    void NormalizeWithPeakSmoothing(double factor) noexcept;

    double NormalizeMeterValue(double amplitude) const noexcept
    {
        return Clamp(Map(amplitude, _GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, 0., 1.), 0., 1.);
    }

public:
    const State * _State;
    const GraphSettings * _GraphSettings;

    uint32_t _SampleRate;
    double _NyquistFrequency;
    std::vector<MeterValue> _MeterValues;

    const WindowFunction * _WindowFunction;
    const WindowFunction * _BrownPucketteKernel;

    FFTAnalyzer * _FFTAnalyzer;
    CQTAnalyzer * _CQTAnalyzer;
    SWIFTAnalyzer * _SWIFTAnalyzer;
    AnalogStyleAnalyzer * _AnalogStyleAnalyzer;

    FrequencyBands _FrequencyBands;

private:
    const double Amax  = M_SQRT1_2;
    const double dBCorrection = -20. * ::log10(Amax); // 3.01;
};

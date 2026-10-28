
/** $VER: AudioProcessor.h (2026.10.28)P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Analyzer.h"
#include "FrequencyBand.h"
#include "State.h"
#include "WindowFunctions.h"

/// <summary>
/// Represents a meter measurement.
/// </summary>
struct measurement_t
{
    measurement_t(Channels channel, const WCHAR * _channelName) noexcept : Channel(channel), ChannelName(_channelName) { }

    Channels Channel;
    std::wstring ChannelName;
};

/// <summary>
/// Represents a peak meter measurement.
/// </summary>
struct peak_measurement_t : measurement_t
{
    peak_measurement_t(Channels channel, const WCHAR * channelName) noexcept : measurement_t(channel, channelName)
    {
        HoldTime = 0.;
        FallRate = 0.;
        Opacity  = 1.;

        RMSTotal = 0.;

        Peak = -std::numeric_limits<double>::infinity();
        NormalizedPeak = 0.;
        MaxNormalizedPeak = 0.;

        RMS = 0.;
        NormalizedRMS = 0.;
    }

    // User settings
    double HoldTime;            // Peak hold time, in seconds [0, 2]
    double FallRate;            // Peak fall rate, in dB/s [0, 240]
    double Opacity;             // [0, 1]

    // Measurements
    double RMSTotal;            // RMS value for the current RMS window.

    double Peak;                // in dBFS
    double NormalizedPeak;      // [0, 1], Normalized and smoothed value used for rendering
    double MaxNormalizedPeak;   // [0, 1], Normalized and smoothed value used for rendering

    double RMS;                 // in dBFS
    double NormalizedRMS;       // [0, 1], Normalized and smoothed value used for rendering
};

/// <summary>
/// Represents the analysis of the sample data.
/// </summary>
class audio_processor_t
{
public:
    audio_processor_t(state_t & state) noexcept : _State(state), _SampleRate(), _ChannelCount(), _ChannelConfig() { };

    audio_processor_t(const audio_processor_t &) = delete;
    audio_processor_t(audio_processor_t &&) = delete;

    audio_processor_t & operator=(const audio_processor_t &) = delete;
    audio_processor_t & operator=(audio_processor_t &&) = delete;

    virtual ~audio_processor_t() noexcept { Reset(); };

    virtual void Configure(const graph_options_t * options) noexcept;
    virtual void Process(const audio_chunk & chunk) noexcept;
    virtual void Reset() noexcept;
    virtual void UpdatePeaks(bool isStopped) noexcept;

    void ResetFFTPeaks() noexcept;

    static double GetAcousticWeight(double x, WeightingType weightingType, double weightAmount) noexcept;

    static std::unique_ptr<audio_processor_t> Create(state_t & state) noexcept;

private:
    void CreateAnalyzer() noexcept;

    // Spectrum
    void SpectrumProcessing(const audio_chunk & chunk) noexcept;

    void GenerateLinearFrequencyBands();
    void GenerateOctaveFrequencyBands();
    void GenerateAveePlayerFrequencyBands();
    void GenerateMelFrequencyBands();
    void GenerateDecadeAlignedFrequencyBands();

    void ApplyAcousticWeighting() noexcept;

    double GetWeight(double frequency) const noexcept;

    static inline double GetFrequencyTilt(double x, double amount, double offset) noexcept;
    static inline double Equalize(double x, double amount, double depth, double offset) noexcept;

    void Normalize() noexcept;
    void NormalizeWithAverageSmoothing(double factor) noexcept;
    void NormalizeWithPeakSmoothing(double factor) noexcept;

public:
    const state_t & _State;
    const graph_options_t * _Options;

    audio_chunk_impl _Chunk;

    uint32_t _SampleRate;
    uint32_t _ChannelCount;
    uint32_t _ChannelConfig;
    uint32_t _ChannelMask;

    double _NyquistFrequency;

    std::unique_ptr<analyzer_t> _Analyzer;

    std::unique_ptr<window_function_t> _WindowFunction;
    std::unique_ptr<window_function_t> _BrownPucketteKernel;

    frequency_bands_t _FrequencyBands;

    std::wstring _DebugText;

    static const uint32_t ChannelPairs[6];

protected:
    msc::chrono_t _Chrono;
};


/** $VER: MeteringProcessor.h (2026.10.28) P. Stuer - Implements an audio processor for the metering visualizations. **/

#pragma once

#include "AudioProcessor.h"
#include "State.h"

#include <sdk/audio_chunk.h>

#include <numbers>

namespace Metering
{

/// <summary>
/// Represents a peak meter measurement.
/// </summary>
struct peak_measurement_t : ::measurement_t
{
    peak_measurement_t(Channels channel, const WCHAR * channelName) noexcept : ::measurement_t(channel, channelName)
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

class processor_t final : public audio_processor_t
{
public:
    processor_t(state_t & state) : audio_processor_t(state) { }

    processor_t(const processor_t &) = delete;
    processor_t(processor_t &&) = delete;

    processor_t & operator=(const processor_t &) = delete;
    processor_t & operator=(processor_t &&) = delete;

    virtual ~processor_t() = default;

public:
    void Configure(const graph_options_t * options) noexcept override final;
    void Reset() noexcept override final;
    void Process(const audio_chunk & chunk) noexcept override final;
    void UpdatePeaks(bool isStopped) noexcept override final;

private:
    void InitializeMeasurements(uint32_t channelMask) noexcept;
    void ResetPeakMeasurements() noexcept;
    void ResetRMSDependentValues() noexcept;

    double NormalizeLLevelValue(double level) const noexcept
    {
        return msc::Map(level, -1., 1., 0., 1.);
    }

    double NormalizeValue(double amplitude) const noexcept
    {
        return std::clamp(msc::Map(amplitude, _Options->_AmplitudeLo, _Options->_AmplitudeHi, 0., 1.), 0., 1.);
    }

    double SmoothValue(double value, double smoothedValue) const noexcept
    {
        switch (_State._SmoothingMethod)
        {
            default:

            case SmoothingMethod::None:
                return value;

            case SmoothingMethod::Average:
                return std::clamp((smoothedValue * _State._SmoothingFactor) + (value * (1. - _State._SmoothingFactor)), 0., 1.);

            case SmoothingMethod::Peak:
                return std::clamp(std::max(smoothedValue * _State._SmoothingFactor, value), 0., 1.);
        }
    }

public:
    audio_chunk_impl _Chunk;

    // Peak meter
    std::vector<peak_measurement_t> _PeakMeasurements;
    uint32_t _SelectedChannels = 0;

    double _RMSTimeElapsed = 0.; // Elapsed time in the current RMS window (in seconds).
    size_t _RMSFrameCount  = 0;  // Number of frames used in the current RMS window.

    // Balance Meter
    double _Left    = 0.;   // [-1, 1]
    double _Right   = 0.;   // [-1, 1]

    double _Mid     = 0.;   // [-1, 1]
    double _Side    = 0.;   // [-1, 1]

    double _Balance = 0.5;  // [0, 1], 0.5 = Center
    double _Phase   = 0.5;  // [0, 1], 0.5 = Center

    static constexpr double Amax = 1. / std::numbers::sqrt2;
    const double dBCorrection = -20. * std::log10(Amax); // 3.01 dB;
};

}

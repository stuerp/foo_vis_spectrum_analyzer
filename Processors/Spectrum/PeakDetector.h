
/** $VER: PeakDetector.h (2026.11.11) P. Stuer - Implements a peak detector for the spectrum visualizations. **/

#pragma once

#include <span>
#include <vector>

#include "FrequencyBand.h"

namespace Peak
{

struct result_t
{
    size_t BandIndex  = 0;
    double Value      = 0.; // Band value
    double Frequency  = 0.; // Interpolated frequency
    double Prominence = 0.; // Height above the surrounding values
};

class detector_t
{
public:
    std::vector<result_t> Process(std::span<const frequency_band_t> bands);

private:
    inline double DetermineProminence(std::span<const frequency_band_t> bands, size_t index, size_t radius) const noexcept;
    inline double InterpolatePeakFrequency(const frequency_band_t & left, const frequency_band_t & center, const frequency_band_t & right) const noexcept;
    inline bool IsFarEnoughFrom(const result_t & candidate, const result_t & accepted) const noexcept;

public:
    size_t MaxPeakCount         = 10;       // The maximum number of peaks that will be detected.
    double MinPeakLevel         = 0.02;     // [0, 1], Minimum normalized level before a frequency is considered a local maximum.
    double MinPeakProminence    = 0.015;    // [0, 1], Minimum required prominence of a frequency to be considered a local maximum.
    size_t BandSpan             = 4;        // Number of frequency bands on each side of the candidate band that will be evaluated when detecting a local maximum.
    double MinPeakDistance      = 20.0;     // Hz, Minimum required distance between two peaks.
    bool InterpolateFrequency   = true;     // True if the candidate frequency should be interpolated with its neighbors.
};

}

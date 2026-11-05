
/** $VER: PeakDetector.h (2026.11.05) P. Stuer - Implements a peak detector for the spectrum visualizations. **/

#pragma once

#include <span>
#include <vector>

#include "FrequencyBand.h"

namespace Peak
{

struct result_t
{
    std::size_t BandIndex {};
    double Value;           // Band value
    double Frequency {};    // Interpolated frequency
    double Difference {};   // Height above the surrounding values
};

class detector_t
{
public:
    std::vector<result_t> Process(std::span<const frequency_band_t> bands);

private:
    inline double DetermineDifference(std::span<const frequency_band_t> bands, std::size_t index, std::size_t radius) noexcept;
    inline double InterpolatePeakFrequency(const frequency_band_t & left, const frequency_band_t & center, const frequency_band_t & right) noexcept;
    inline bool IsFarEnoughFrom(const result_t& candidate, const result_t& accepted) noexcept;

public:
    std::size_t MaxPeakCount        = 10;

    double MinValue                 = 0.02;     // Minimum magnitude before a frequency band is taken into account.
    double MinValueDifference       = 0.015;    // Minimum required difference to be considered a local maximum.
    std::size_t BandSpan            = 4;        // Number of frequency bands on each side evaluated when detecting a local maximum.
    double MinFrequencyDifference   = 20.0;     // Minimum required distance between two peaks.
    bool InterpolateFrequency       = true;     // True if the candidate frequency should be interpolated with its neighbors.
};

}

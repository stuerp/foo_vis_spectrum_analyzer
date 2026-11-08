
/** $VER: PeakDetector.h (2026.11.05) P. Stuer - Implements a peak detector for the spectrum visualizations. **/

#pragma once

#include <span>
#include <vector>

#include "FrequencyBand.h"

namespace Peak
{

struct result_t
{
    size_t BandIndex {};
    double Value;           // Band value
    double Frequency {};    // Interpolated frequency
    double Prominence {};   // Height above the surrounding values
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
    size_t MaxPeakCount = 10;

    double MinValue             = 0.02;     // [0, 1], Minimum magnitude before a frequency band is taken into account.
    double MinProminence        = 0.015;    // [0, 1], Minimum required prominence to be considered a local maximum.
    size_t BandSpan             = 4;        // Number of frequency bands on each side evaluated when detecting a local maximum.
    double MinFrequencySpacing  = 20.0;     // Hz, Minimum required distance between two peaks.
    bool InterpolateFrequency   = true;     // True if the candidate frequency should be interpolated with its neighbors.
};

}

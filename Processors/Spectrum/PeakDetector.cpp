
/** $VER: PeakDetector.cpp (2026.11.11) P. Stuer - Implements a peak detector for the spectrum visualizations. **/

#include <pch.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include "PeakDetector.h"

namespace Peak
{
    /// <summary>
    /// Determines the local maxima in the spectrum.
    /// </summary>
    std::vector<result_t> detector_t::Process(std::span<const frequency_band_t> frequencyBands)
    {
        std::vector<result_t> Candidates;

        if (frequencyBands.size() < 3 || MaxPeakCount == 0)
            return Candidates;

        Candidates.reserve(frequencyBands.size() / 4);

        for (size_t i = 1; i + 1 < frequencyBands.size(); ++i)
        {
            const auto & Left   = frequencyBands[i - 1];
            const auto & Center = frequencyBands[i];
            const auto & Right  = frequencyBands[i + 1];

            if (Center.Value < MinPeakLevel)
                continue;

            const bool IsLocalMaximum = (Center.Value > Left.Value) && (Center.Value >= Right.Value);

            if (!IsLocalMaximum)
                continue;

            const double Prominence = DetermineProminence(frequencyBands, i, BandSpan);

            if (Prominence < MinPeakProminence)
                continue;

            double Frequency = Center.Center;

            if (InterpolateFrequency)
                Frequency = InterpolatePeakFrequency(Left, Center, Right);

            Candidates.push_back
            ({
                .BandIndex  = i,
                .Value      = Center.Value,
                .Frequency  = Frequency,
                .Prominence = Prominence
            });
        }

        // Sort the candidates in descending order of importance.
        std::ranges::sort(Candidates, [](const result_t & a, const result_t & b)
        {
            if (a.Value != b.Value)
                return a.Value > b.Value;

            if (a.Prominence != b.Prominence)
                return a.Prominence > b.Prominence;

            return a.Frequency < b.Frequency;
        });

        // Process the candidates in descending order of importance. Retain the top candidates that are spaced apart far enough.
        std::vector<result_t> Result;

        Result.reserve(std::min(MaxPeakCount, Candidates.size()));

        for (const auto & Candidate : Candidates)
        {
            const bool IsFarEnough = std::ranges::all_of(Result, [&](const result_t & acceptedResult)
            {
                return IsFarEnoughFrom(Candidate, acceptedResult);
            });

            if (!IsFarEnough)
                continue;

            Result.push_back(Candidate);

            if (Result.size() == MaxPeakCount)
                break;
        }

        // Return the result sorted by increasing band index for optimal rendering.
        std::ranges::sort(Result, {}, &result_t::BandIndex);

        return Result;
    }

    /// <summary>
    /// Determines the level difference of the specified frequency.
    /// </summary>
    inline double detector_t::DetermineProminence(std::span<const frequency_band_t> bands, size_t bandIndex, size_t bandSpan) const noexcept
    {
        const auto Left  = (size_t) std::max<int64_t>(0,               (int64_t) bandIndex - (int64_t) bandSpan);
        const auto Right =          std::min<size_t>(bands.size() - 1, bandIndex + bandSpan);

        double LeftMin = bands[bandIndex].Value;

        for (size_t i = Left; i < bandIndex; ++i)
            LeftMin = std::min(LeftMin, bands[i].Value);

        double RightMin = bands[bandIndex].Value;

        for (size_t i = bandIndex + 1; i <= Right; ++i)
            RightMin = std::min(RightMin, bands[i].Value);

        const double Min = std::max(LeftMin, RightMin);

        return std::max(0., bands[bandIndex].Value - Min);
    }

    /// <summary>
    /// Interpolates the candidate frequency with its neighbors.
    /// </summary>
    inline double detector_t::InterpolatePeakFrequency(const frequency_band_t & left, const frequency_band_t & center, const frequency_band_t & right) const noexcept
    {
        const double y0 = left.Value;
        const double y1 = center.Value;
        const double y2 = right.Value;

        const double Value = y0 - (y1 * 2.) + y2;

        if (!std::isfinite(Value) || (std::abs(Value) <= std::numeric_limits<double>::epsilon()))
            return center.Center;

        const double Offset = std::clamp((y0 - y2) * 0.5 / Value, -1., 1.);

        double Frequency = center.Center;

        if (Offset < 0.)
            Frequency += Offset * (center.Center - left.Center);
        else
            Frequency += Offset * (right.Center - center.Center);

        return std::clamp(Frequency, left.Center, right.Center);
    }

    /// <summary>
    /// Returns true if the peak is far enough from the last accepted peak.
    /// </summary>
    inline bool detector_t::IsFarEnoughFrom(const result_t & candidate, const result_t & accepted) const noexcept
    {
        const double Distance = std::abs(candidate.Frequency - accepted.Frequency);

        if ((MinPeakDistance > 0.) && (Distance < MinPeakDistance))
            return false;

        return true;
    }
}

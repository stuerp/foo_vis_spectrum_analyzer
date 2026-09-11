
/** $VER: Aggregator.h (2026.09.02) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <complex>
#include <vector>

/// <summary>
/// Declares a base class for the FFT coefficient aggregators.
/// </summary>
class aggregator_t
{
public:
    virtual ~aggregator_t() = default;

    virtual inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept = 0;
};

/// <summary>
/// Implements an aggregator that returns the minimum value of the specified bin range.
/// </summary>
class min_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        double Value = std::numeric_limits<double>::max();

        for (size_t i = 0; i < size; ++i)
        {
            Value = std::min(Value, std::abs(data[binIndex]));

            ++binIndex;

            if (binIndex == maxBins)
                binIndex = 0;
        }

        return Value;
    }
};

/// <summary>
/// Implements an aggregator that returns the maximum value of the specified bin range.
/// </summary>
class max_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        double Value = 0.;

        for (size_t i = 0; i < size; ++i)
        {
            Value = std::max(Value, std::abs(data[binIndex]));

            ++binIndex;

            if (binIndex == maxBins)
                binIndex = 0;
        }

        return Value;
    }
};

/// <summary>
/// Implements an aggregator that returns the sum of the specified bin range.
/// </summary>
class sum_aggregator_t final : public aggregator_t
{
public:
    sum_aggregator_t(bool smoothGainTransition) : _SmoothGainTransition(smoothGainTransition) { }

    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        double Value = 0.;

        for (size_t i = 0; i < size; ++i)
        {
            Value += std::abs(data[binIndex]);

            ++binIndex;

            if (binIndex == maxBins)
                binIndex = 0;
        }

        if (_SmoothGainTransition)
            Value /= (double) size;

        return Value;
    }

private:
    bool _SmoothGainTransition;
};

/// <summary>
/// Implements an aggregator that returns the average of the specified bin range.
/// </summary>
class avg_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        assert(size != 0);

        double Value = 0.;

        for (size_t i = 0; i < size; ++i)
        {
            Value += std::abs(data[binIndex]);

            ++binIndex;

            if (binIndex == maxBins)
                binIndex = 0;
        }

        return Value / (double) size;
    }
};

/// <summary>
/// Implements an aggregator that returns the RMS of the specified bin range.
/// </summary>
class rms_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        assert(size != 0);

        double Value = 0.;

        for (size_t i = 0; i < size; ++i)
        {
            Value += std::norm(data[binIndex]);

            ++binIndex;

            if (binIndex == maxBins)
                binIndex = 0;
        }

        return std::sqrt(Value / (double) size);
    }
};

/// <summary>
/// Implements an aggregator that returns the summed RMS of the specified bin range.
/// </summary>
class rms_sum_aggregator_t final : public aggregator_t
{
public:
    rms_sum_aggregator_t(bool smoothGainTransition) : _SmoothGainTransition(smoothGainTransition) { }

    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        double Value = 0.;

        for (size_t i = 0; i < size; ++i)
        {
            Value += std::norm(data[binIndex]);

            ++binIndex;

            if (binIndex == maxBins)
                binIndex = 0;
        }

        if (_SmoothGainTransition)
            Value /= (double) size;

        return std::sqrt(Value);
    }

private:
    bool _SmoothGainTransition;
};

/// <summary>
/// Implements an aggregator that returns the median of the specified bin range.
/// </summary>
class median_aggregator_t final : public aggregator_t
{
public:
    median_aggregator_t(size_t maxBins)
    {
        Values.reserve(maxBins);
    }

    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        Values.clear();

        for (size_t i = 0; i < size; ++i)
        {
            Values.push_back(std::abs(data[binIndex]));

            ++binIndex;

            if (binIndex == maxBins)
                binIndex = 0;
        }

        const double Value = Median(Values);

        return Value;
    }

private:
    /// <summary>
    /// Calculates the median.
    /// </summary>
    double Median(std::vector<double> & data) const noexcept
    {
        if (data.empty())
            return std::numeric_limits<double>::quiet_NaN();

        if (data.size() == 1)
            return data[0];

        std::sort(data.begin(), data.end());

        const size_t Mid = data.size() / 2;

        return (data.size() & 1) ? data[Mid] : (data[Mid - 1] + data[Mid]) / 2.;
    }

private:
    std::vector<double> Values;
};

/// <summary>
/// Implements an aggregator that returns the last value of the specified bin range.
/// </summary>
class default_aggregator_t : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t size, size_t binIndex, size_t maxBins) noexcept final
    {
        binIndex = msc::Wrap(binIndex + size, maxBins);

        double Value = std::abs(data[binIndex]);

        return Value;
    }
};

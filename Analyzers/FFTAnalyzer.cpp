
/** $VER: FFTAnalyzer.cpp (2026.08.30) P. Stuer - Based on TF3RDL's FFT analyzer, https://codepen.io/TF3RDL/pen/poQJwRW **/

#include "pch.h"

#include "FFTAnalyzer.h"
#include "Log.h"

#include <execution>

#pragma hdrstop

/// <summary>
/// Destroys this instance.
/// </summary>
fft_analyzer_t::~fft_analyzer_t()
{ }

/// <summary>
/// Initializes an instance of the class.
/// </summary>
fft_analyzer_t::fft_analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, size_t fftSize, const window_function_t & windowFunction, const window_function_t & brownPucketteKernel) noexcept : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction), _BrownPucketteKernel(brownPucketteKernel)
{
    _FFTSize = fftSize;

    _FFT.Initialize(_FFTSize);

    // Create the input ring buffer.
    _InputRing.resize(_FFTSize, (audio_sample)0.);
    _Next = 0;

    _TimeData.resize(_FFTSize);
    _FreqData.resize(_FFTSize);
}

/// <summary>
/// Calculates the transform and returns the frequency bands.
/// </summary>
bool fft_analyzer_t::AnalyzeSamples(const audio_sample * frameData, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept
{
//  const auto Start = std::chrono::steady_clock::now();

    Add(frameData, frameCount, selectedChannels);

    Transform();

    switch (_State->_MappingMethod)
    {
        default:

        case Mapping::Standard:
            MapCoefficients(frequencyBands);
            break;

        case Mapping::TriangularFilterBank:
            MapCoefficientsUsingTFB(frequencyBands);
            break;

        case Mapping::BrownPuckette:
            MapCoefficientsUsingBP(frequencyBands);
            break;
    }

//  const auto Finish = std::chrono::steady_clock::now();

//  Log.AtInfo().Write("%6d ms", std::chrono::duration_cast<std::chrono::microseconds>(Finish - Start));

    return true;
}

/// <summary>
/// Adds multiple samples to the input buffer.
/// It assumes that the buffer contains frames of sample data with a reading for each channel specified in the channel configuration of the chunk.
/// E.g. for 2 channels: Left(0), Right(0), Left(1), Right(1) ... Left(n), Right(n)
/// </summary>
void fft_analyzer_t::Add(const audio_sample * samples, size_t frameCount, uint32_t selectedChannels) noexcept
{
    if (samples == nullptr)
        return;

    const size_t SampleCount = frameCount * _ChannelCount;

    audio_sample * const p = _InputRing.data();

    for (size_t i = 0; i < SampleCount; i += _ChannelCount)
    {
        // Downmix the selected channels to mono before adding the sample to the input ring buffer.
        p[_Next] = Downmix(&samples[i], selectedChannels);

        _Next = (_Next + 1) % _InputRing.size();
    }
}

/// <summary>
/// Calculates the Fast Fourier Transform and updates the frequency data.
/// </summary>
void fft_analyzer_t::Transform() noexcept
{
    double Norm = 0.;

    // Fill the FFT buffer from the input ring buffer with Time domain data, apply the windowing function and determine the norm.
    {
        size_t i = (_Next - _FFTSize) % _InputRing.size();
        size_t j = 0;

        const audio_sample * const p = _InputRing.data();

        for (auto & Iter : _TimeData)
        {
            const double WindowFactor = _WindowFunction(msc::Map(j, (size_t)0, _FFTSize - 1, -1., 1.));

            Iter = std::complex<double>(p[i] * WindowFactor, 0.);

            i = (i + 1) % _InputRing.size();

            Norm += WindowFactor;
            j++;
        }
    }

    // Normalize the Time domain data.
    {
        const double Factor = (double)_FFTSize / Norm; // * M_SQRT2;

        std::transform(std::execution::par_unseq, _TimeData.begin(), _TimeData.end(), _TimeData.begin(), [Factor](std::complex<double> x)
        {
            return x * Factor;
        });
    }

    // Transform the data from the Time domain to the Frequency domain.
    _FFT.Transform(_TimeData, _FreqData);

    // Normalize the Frequency domain data.
    {
        const double Factor = 2. / (double)_FFTSize;

        std::transform(std::execution::par_unseq, _FreqData.begin(), _FreqData.end(), _FreqData.begin(), [Factor](std::complex<double> x)
        {
            return x * Factor;
        });
    }
}

#define v2

#ifdef v1
void fft_analyzer_t::MapCoefficients(frequency_bands_t & freqBands) const noexcept
{
    const bool IsRMS       = (_State->_AggregationMethod == AggregationMethod::RMS || _State->_AggregationMethod == AggregationMethod::RMSSum);
    const bool IsMedian    = _State->_AggregationMethod == AggregationMethod::Median;
    const bool UseBandGain = (_State->_SmoothGainTransition && (_State->_AggregationMethod == AggregationMethod::Sum || _State->_AggregationMethod == AggregationMethod::RMSSum));
    const bool IsAverage   = ((_State->_AggregationMethod == AggregationMethod::Average || _State->_AggregationMethod == AggregationMethod::RMS) || UseBandGain);

    std::vector<double> Values;

    for (frequency_band_t & fb : freqBands)
    {
        const double BandGain = UseBandGain ? std::hypot(1, std::pow(((fb.Hi - fb.Lo) * (double)(_FreqData.size() - 1) / (double)_SampleRate), (IsRMS ? 0.5 : 1.))) : 1.;

        double LoIdx = HzToBinIndex(fb.Lo, _FreqData.size());
        double HiIdx = HzToBinIndex(fb.Hi, _FreqData.size());

        LoIdx = (_State->_SmoothLowerFrequencies ? std::round(LoIdx) + 1. : std::ceil(LoIdx));
        HiIdx = (_State->_SmoothLowerFrequencies ? std::round(HiIdx) - 1. : std::floor(HiIdx));

        if (LoIdx <= HiIdx)
        {
            HiIdx -= std::max(HiIdx - LoIdx - (double)_FreqData.size(), 0.);

            double Value = (_State->_AggregationMethod == AggregationMethod::Minimum) ? DBL_MAX : 0.;

            Values.clear();

            int Count = 0;

            for (auto Idx = LoIdx; Idx <= HiIdx; ++Idx)
            {
                const size_t BinIdx = msc::Wrap((size_t)Idx, _FreqData.size());

                const double Magnitude = std::abs(_FreqData[BinIdx]);

                switch (_State->_AggregationMethod)
                {
                    case AggregationMethod::Minimum:
                        Value = std::min(Magnitude, Value);
                        break;

                    case AggregationMethod::Maximum:
                        Value = std::max(Magnitude, Value);
                        break;

                    case AggregationMethod::Sum:
                    case AggregationMethod::Average:
                        Value += Magnitude;
                        break;

                    case AggregationMethod::RMS:
                    case AggregationMethod::RMSSum:
                        Value += Magnitude * Magnitude;
                        break;

                    case AggregationMethod::Median:
                        Values.push_back(Magnitude);
                        break;

                    default:
                        Value = Magnitude;
                }

                ++Count;
            }

            if (IsAverage && (Count != 0))
                Value /= Count;
            else
            if (IsMedian)
                Value = Median(Values);

            fb.RawValue = (IsRMS ? std::sqrt(Value) : Value) * BandGain;
        }
        else
        {
            const double Index = HzToBinIndex(fb.Mid, _FreqData.size());

            fb.RawValue = std::fabs(Interpolate(_FreqData, Index, _State->_KernelSize)) * BandGain;
        }
    }
}

/// <summary>
/// Calculates the median.
/// </summary>
double fft_analyzer_t::Median(std::vector<double> & data) noexcept
{
    if (data.empty())
        return std::numeric_limits<double>::quiet_NaN();

    if (data.size() == 1)
        return data[0];

    std::sort(data.begin(), data.end());

    const size_t Mid = data.size() / 2;

    return (data.size() & 1) ? data[Mid] : (data[Mid - 1] + data[Mid]) / 2.;
}
#endif

#ifdef v2
class aggregator_t
{
public:
    virtual ~aggregator_t() = default;

    virtual inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept = 0;
};

class min_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        double Value = std::numeric_limits<double>::max();

        for (size_t i = 0; i < count; ++i)
        {
            Value = std::min(Value, std::abs(data[binIndex]));

            ++binIndex;

            if (binIndex == binCount)
                binIndex = 0;
        }

        return Value;
    }
};

class max_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        double Value = 0.;

        for (size_t i = 0; i < count; ++i)
        {
            Value = std::max(Value, std::abs(data[binIndex]));

            ++binIndex;

            if (binIndex == binCount)
                binIndex = 0;
        }

        return Value;
    }
};

class sum_aggregator_t final : public aggregator_t
{
public:
    sum_aggregator_t(bool smoothGainTransition) : _SmoothGainTransition(smoothGainTransition) { }

    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        double Value = 0.;

        for (size_t i = 0; i < count; ++i)
        {
            Value += std::abs(data[binIndex]);

            ++binIndex;

            if (binIndex == binCount)
                binIndex = 0;
        }

        if (_SmoothGainTransition)
            Value /= (double) count;

        return Value;
    }

private:
    bool _SmoothGainTransition;
};

class avg_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        assert(count != 0);

        double Value = 0.;

        for (size_t i = 0; i < count; ++i)
        {
            Value += std::abs(data[binIndex]);

            ++binIndex;

            if (binIndex == binCount)
                binIndex = 0;
        }

        return Value / (double) count;
    }
};

class rms_aggregator_t final : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        assert(count != 0);

        double Value = 0.;

        for (size_t i = 0; i < count; ++i)
        {
            Value += std::norm(data[binIndex]);

            ++binIndex;

            if (binIndex == binCount)
                binIndex = 0;
        }

        return std::sqrt(Value / (double) count);
    }
};

class rms_sum_aggregator_t final : public aggregator_t
{
public:
    rms_sum_aggregator_t(bool smoothGainTransition) : _SmoothGainTransition(smoothGainTransition) { }

    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        double Value = 0.;

        for (size_t i = 0; i < count; ++i)
        {
            Value += std::norm(data[binIndex]);

            ++binIndex;

            if (binIndex == binCount)
                binIndex = 0;
        }

        if (_SmoothGainTransition)
            Value /= (double) count;

        return std::sqrt(Value);
    }

private:
    bool _SmoothGainTransition;
};

class median_aggregator_t final : public aggregator_t
{
public:
    median_aggregator_t(size_t binCount)
    {
        Values.reserve(binCount);
    }

    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        Values.clear();

        for (size_t i = 0; i < count; ++i)
        {
            Values.push_back(std::abs(data[binIndex]));

            ++binIndex;

            if (binIndex == binCount)
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

class default_aggregator_t : public aggregator_t
{
public:
    inline double operator()(const std::vector<std::complex<double>> & data, size_t count, size_t binIndex, size_t binCount) noexcept final
    {
        double Value = 0.;

        // The value of the last bin wins.
        for (size_t i = 0; i < count; ++i)
        {
            Value = std::abs(data[binIndex]);

            ++binIndex;

            if (binIndex == binCount)
                binIndex = 0;
        }

        return Value;
    }
};

/// <summary>
/// Maps FFT power-spectrum coefficients onto the frequency bands.
/// </summary>
void fft_analyzer_t::MapCoefficients(frequency_bands_t & freqBands) const noexcept
{
    const auto Method                 = _State->_AggregationMethod;
    const bool SmoothLowerFrequencies = _State->_SmoothLowerFrequencies;
    const bool SmoothGainTransition   = _State->_SmoothGainTransition && (Method == AggregationMethod::Sum || Method == AggregationMethod::RMSSum);

    const bool IsRMS = (Method == AggregationMethod::RMS) || (Method == AggregationMethod::RMSSum);

    const size_t BinCount = _FreqData.size();

    if (BinCount == 0)
        return;

    const auto BandWidthScale = (double)(BinCount - 1) / (double)_SampleRate;

    std::unique_ptr<aggregator_t> Aggregate;

    switch (Method)
    {
        case AggregationMethod::Minimum:
            Aggregate = std::make_unique<min_aggregator_t>();
            break;

        case AggregationMethod::Maximum:
            Aggregate = std::make_unique<max_aggregator_t>();
            break;

        case AggregationMethod::Sum:
            Aggregate = std::make_unique<sum_aggregator_t>(_State->_SmoothGainTransition);
            break;

        case AggregationMethod::Average:
            Aggregate = std::make_unique<avg_aggregator_t>();
            break;

        case AggregationMethod::RMS:
            Aggregate = std::make_unique<rms_aggregator_t>();
            break;

        case AggregationMethod::RMSSum:
            Aggregate = std::make_unique<rms_sum_aggregator_t>(_State->_SmoothGainTransition);
            break;

        case AggregationMethod::Median:
            Aggregate = std::make_unique<median_aggregator_t>(BinCount);
            break;

        default:
            Aggregate = std::make_unique<default_aggregator_t>();
            break;
    }

    for (frequency_band_t & fb : freqBands)
    {
        assert(fb.Lo <= fb.Mid && fb.Mid <= fb.Hi);

        double BandGain = 1.;

        if (SmoothGainTransition)
        {
            const double WidthInBins   = (fb.Hi - fb.Lo) * BandWidthScale;
            const double AdjustedWidth = IsRMS ? std::sqrt(WidthInBins) : WidthInBins;

            BandGain = std::hypot(1., AdjustedWidth);
        }

        const double LoIdx = HzToBinIndex(fb.Lo, BinCount);
        const double HiIdx = HzToBinIndex(fb.Hi, BinCount);

        const auto First = std::max((ptrdiff_t) (SmoothLowerFrequencies ? std::round(LoIdx) + 1. : std::ceil (LoIdx)), (ptrdiff_t) 0);
        const auto Last =           (ptrdiff_t) (SmoothLowerFrequencies ? std::round(HiIdx) - 1. : std::floor(HiIdx));

        if (First > Last)
        {
            const double Index = HzToBinIndex(fb.Mid, BinCount);

            fb.RawValue = std::abs(Interpolate(_FreqData, Index, _State->_KernelSize)) * BandGain;
            continue;
        }

        const auto RequestedCount = (size_t) (Last - First + 1);
        const auto Count = std::min(RequestedCount, BinCount);

        size_t BinIndex = msc::Wrap((size_t) First, BinCount);

        const double Value = (*Aggregate)(_FreqData, Count, BinIndex, BinCount);

        fb.RawValue = Value * BandGain;
    }
}
#endif

#ifdef x1
void fft_analyzer_t::MapCoefficientsUsingTFB(frequency_bands_t & freqBands) const noexcept
{
    const double N = (double) _FreqData.size();

    const double HzToBin = N / _SampleRate;

    for (frequency_band_t & fb : freqBands)
    {
        double Sum = 0.;

        const auto MinBin = std::min(fb.Lo, fb.Hi) * HzToBin;
        const auto MidBin =          fb.Mid        * HzToBin;
        const auto MaxBin = std::max(fb.Lo, fb.Hi) * HzToBin;

        const double OverflowCompensation = std::max(0., MaxBin - MinBin - N);

        {
            const auto Start = std::floor(MidBin);
            const auto End   = std::floor(MinBin + OverflowCompensation);

            for (double i = Start; i >= End; --i)
                Sum += std::pow(std::abs(_FreqData[(size_t) msc::Wrap(i, N)]) * std::max(msc::Map(i, MinBin, MidBin, 0., 1.), 0.), 2.);
        }

        {
            const auto Start = std::ceil(MidBin);
            const auto End   = std::ceil(MaxBin - OverflowCompensation);

            for (double i = Start; i <= End; ++i)
                Sum += std::pow(std::abs(_FreqData[(size_t) msc::Wrap(i, N)]) * std::max(msc::Map(i, MaxBin, MidBin, 0., 1.), 0.), 2.);
        }

        fb.RawValue = std::sqrt(Sum);
    }
}
#endif

#ifdef x2
void fft_analyzer_t::MapCoefficientsUsingTFB(frequency_bands_t & freqBands) const noexcept
{
    if (_FreqData.empty() || !std::isfinite(_SampleRate) || _SampleRate <= 0.)
    {
        for (frequency_band_t & fb : freqBands)
            fb.RawValue = 0.;

        return;
    }

    const size_t BinCount = _FreqData.size();
    const double N        = (double) (BinCount);
    const double HzToBin  = N / _SampleRate;

    for (frequency_band_t & fb : freqBands)
    {
        if (!std::isfinite(fb.Lo)  || !std::isfinite(fb.Mid) || !std::isfinite(fb.Hi))
        {
            fb.RawValue = 0.;

            continue;
        }

        const double MinBin = std::min(fb.Lo, fb.Hi) * HzToBin;
        const double MidBin =          fb.Mid        * HzToBin;
        const double MaxBin = std::max(fb.Lo, fb.Hi) * HzToBin;

        // A proper triangular band requires its peak to lie strictly between its lower and upper edges.
        if (!(MinBin < MidBin && MidBin < MaxBin))
            continue;

        const int StartBin = std::max(0, (int) std::ceil(MinBin));
        const int EndBin   = std::min((int) BinCount - 1, (int) std::floor(MaxBin));

        if (StartBin > EndBin)
            continue;

        double WeightedPower = 0.;

        for (int Bin = StartBin; Bin <= EndBin; ++Bin)
        {
            const auto Position = (double) Bin;

            double Weight;

            if (Position <= MidBin)
                Weight = (Position - MinBin) / (MidBin - MinBin);
            else
                Weight = (MaxBin - Position) / (MaxBin - MidBin);

            Weight = std::clamp(Weight, 0., 1.);

            const std::complex<double> & Coefficient = _FreqData[(size_t) Bin];

            WeightedPower += std::norm(Coefficient) * Weight;
        }

        fb.RawValue = std::sqrt(std::max(WeightedPower, 0.));
    }
}
#endif

/// <summary>
/// Maps FFT power-spectrum coefficients onto triangular frequency bands. Assumes the frequency bands are spaced on the Mel scale.
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Mel-frequency_cepstrum</ref>
void fft_analyzer_t::MapCoefficientsUsingTFB(frequency_bands_t & freqBands) const noexcept
{
    if (_FreqData.empty() || !std::isfinite(_SampleRate) || _SampleRate <= 0.)
    {
        for (frequency_band_t & fb : freqBands)
            fb.RawValue = 0.;

        return;
    }

    const size_t BinCount = _FreqData.size();
    const double N        = (double) BinCount;
    const double HzToBin  = N / _SampleRate;

    // Linearly interpolate between adjacent FFT-bin powers.
    const auto InterpolatePower = [&](double index) noexcept
    {
        index = std::clamp(index, 0., N - 1.);

        const size_t LowerBin = (size_t) std::floor(index);
        const size_t UpperBin = std::min(LowerBin + 1, BinCount - 1);

        const double LowerPower = std::norm(_FreqData[LowerBin]);
        const double UpperPower = std::norm(_FreqData[UpperBin]);
        const double Fraction   = index - (double) LowerBin;

        return std::lerp(LowerPower, UpperPower, Fraction);
    };

    for (frequency_band_t & fb : freqBands)
    {
        fb.RawValue = 0.;

        if (!std::isfinite(fb.Lo) || !std::isfinite(fb.Mid) || !std::isfinite(fb.Hi))
            continue;

        const double MinBin = std::min(fb.Lo, fb.Hi) * HzToBin;
        const double MidBin =          fb.Mid        * HzToBin;
        const double MaxBin = std::max(fb.Lo, fb.Hi) * HzToBin;

        // A proper triangular band requires its peak to lie strictly between its lower and upper edges.
        if (!(MinBin < MidBin && MidBin < MaxBin))
            continue;

        const double StartBin = std::max(    0., MinBin);
        const double EndBin   = std::min(N - 1., MaxBin);

        if (StartBin >= EndBin)
            continue;

        double WeightedPower = 0.;

        {
            const auto GetWeight = [&](const double index) noexcept
            {
                double Weight;

                if (index <= MidBin)
                    Weight = (index - MinBin) / (MidBin - MinBin);
                else
                    Weight = (MaxBin - index) / (MaxBin - MidBin);

                return std::clamp(Weight, 0., 1.);
            };

            double Index = StartBin;

            while (Index < EndBin)
            {
                // Split at FFT-bin boundaries and at the triangular band's peak. Within each interval both the interpolated power and triangular weight are continuous.
                double NextIndex = std::min(std::floor(Index) + 1., EndBin);

                if ((Index < MidBin) && (MidBin < NextIndex))
                    NextIndex = MidBin;

                if (!(Index < NextIndex))
                    NextIndex = std::min(Index + 1., EndBin);

                const double MidIndex = (Index + NextIndex) / 2.;

                const double StartValue  = InterpolatePower(Index)     * GetWeight(Index);
                const double MiddleValue = InterpolatePower(MidIndex)  * GetWeight(MidIndex);
                const double EndValue    = InterpolatePower(NextIndex) * GetWeight(NextIndex);

                WeightedPower += (NextIndex - Index) * (StartValue + 4. * MiddleValue + EndValue) / 6.;

                Index = NextIndex;
            }
        }

        fb.RawValue = std::sqrt(std::max(WeightedPower, 0.));
    }
}

/// <summary>
/// Maps FFT power-spectrum coefficients onto the frequency bands. (Brown-Puckette)
/// <ref>https://en.wikipedia.org/wiki/Pitch_detection_algorithm</ref>
void fft_analyzer_t::MapCoefficientsUsingBP(frequency_bands_t & freqBands) const noexcept
{
    if (_FreqData.empty() || !(_SampleRate > 0.))
        return;

    const double N       = (double) _FreqData.size();
    const double HzToBin = N / _SampleRate;
    const double BinToHz = _SampleRate / N;

    for (frequency_band_t & fb : freqBands)
    {
        double re = 0.;
        double im = 0.;

        double Norm = 0.;

        // In Hz
        const double Bandwidth      = std::max(std::abs(fb.Hi - fb.Lo) + (BinToHz * _State->_BandwidthOffset), std::numeric_limits<double>::epsilon());
        // In seconds (Time domain)
        const double WindowDuration = std::min(1. / Bandwidth, HzToBin / _State->_BandwidthCap);
        // In samples (Sample domain)
        const double WindowSize     = std::max(_State->_UseGranularBandwidth ? WindowDuration * _SampleRate : std::min(std::trunc(std::pow(2., std::round(std::log2(WindowDuration * _SampleRate)))), N / _State->_BandwidthCap), 1.);
        // In bins (Frequency domain)
        const double KernelSize     = std::min(_State->_BandwidthAmount * N / WindowSize, N);

        const double Center = fb.Mid * HzToBin;

        const double Half = 0.5 * (KernelSize - 1.);

        const auto Start = (int) std::ceil (Center - Half);
        const auto End   = (int) std::floor(Center + Half);

        for (int i = Start; i <= End; ++i)
        {
            const double NormalizedBinOffset = 2. * ((double) i - Center) / KernelSize; // Within [-1, 1]

            double Weight = _BrownPucketteKernel(NormalizedBinOffset);

            // Improves low-frequency spectral reconstruction by alternating the phase of neighboring interpolation taps. Non-standard, visualization-specific adjustment.
            if ((i & 1) == 0)
                Weight = -Weight;

            Norm += Weight * Weight;

            const auto Index = (size_t) msc::Wrap(i, (int) _FreqData.size());

            re += _FreqData[Index].real() * Weight;
            im += _FreqData[Index].imag() * Weight;
        }

        if (Norm > 0.)
        {
            const double InvNorm = 1. / std::sqrt(Norm);

            re *= InvNorm;
            im *= InvNorm;
        }

        fb.RawValue = std::hypot(re, im);
    }
}

/// <summary>
/// Determines the interpolated magnitude at a fractional index.
/// </summary>
double fft_analyzer_t::Interpolate(const std::vector<std::complex<double>> & fftCoeffs, double index, int kernelSize) const noexcept
{
    std::complex<double> Sum = { 0., 0. };

    const auto Base = (int) std::floor(index);

    auto sinc = [](double x) noexcept
    {
        return (std::abs(x) > std::numeric_limits<double>::epsilon()) ? std::sin(x) / x : 1.;
    };

    for (int i = -kernelSize; i <= kernelSize; ++i)
    {
        const int Index = Base + i;

        // Distance from the interpolation point.
        const double d = (index - Index) * M_PI;

        // Lanczos-a kernel.
        double Weight = sinc(d) * sinc(d / kernelSize);

        // Improves low-frequency spectral reconstruction by alternating the phase of neighboring interpolation taps. Non-standard, visualization-specific adjustment.
        if ((i & 1) == 0)
            Weight = -Weight;

        const auto CoefIdx = (size_t) msc::Wrap(Index, (int) fftCoeffs.size());

        Sum += fftCoeffs[CoefIdx] * Weight;
    }

    return std::abs(Sum);
}

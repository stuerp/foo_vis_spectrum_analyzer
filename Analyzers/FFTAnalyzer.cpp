
/** $VER: FFTAnalyzer.cpp (2026.02.11) P. Stuer - Based on TF3RDL's FFT analyzer, https://codepen.io/TF3RDL/pen/poQJwRW **/

#include "pch.h"
#include "FFTAnalyzer.h"

#include "Log.h"

#include <execution>

#pragma hdrstop

/// <summary>
/// Destroys this instance.
/// </summary>
fft_analyzer_t::~fft_analyzer_t()
{
}

/// <summary>
/// Initializes an instance of the class.
/// </summary>
fft_analyzer_t::fft_analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction, const window_function_t & brownPucketteKernel, size_t fftSize) : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction), _BrownPucketteKernel(brownPucketteKernel)
{
    _FFTSize = fftSize;

    _FFT.Initialize(_FFTSize);

    // Create the ring buffer for the samples.
    _InputRing.resize(_FFTSize, 0.);
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
            AnalyzeSamples(_SampleRate, frequencyBands);
            break;

        case Mapping::TriangularFilterBank:
            AnalyzeSamplesUsingTFB(_SampleRate, frequencyBands);
            break;

        case Mapping::BrownPuckette:
            AnalyzeSamplesUsingBP(_SampleRate, frequencyBands);
            break;
    }

//  const auto Finish = std::chrono::steady_clock::now();

//  Log.AtInfo().Write("%6d ms", std::chrono::duration_cast<std::chrono::milliseconds>(Finish - Start));

    return true;
}

/// <summary>
/// Adds multiple samples to the analyzer buffer.
/// It assumes that the buffer contains frames of sample data with a reading for each channel specified in the channel configuration of the chunk.
/// E.g. for 2 channels: Left(0), Right(0), Left(1), Right(1) ... Left(n), Right(n)
/// </summary>
void fft_analyzer_t::Add(const audio_sample * samples, size_t frameCount, uint32_t selectedChannels) noexcept
{
    if (samples == nullptr)
        return;

    const size_t SampleCount = frameCount * _ChannelCount;

    // Merge the samples of all selected channels into one averaged sample.
    audio_sample * const p = _InputRing.data();

    for (size_t i = 0; i < SampleCount; i += _ChannelCount)
    {
        p[_Next] = AverageSamples(&samples[i], selectedChannels);

        _Next = (_Next + 1) % _InputRing.size();
    }
}

/// <summary>
/// Calculates the Fast Fourier Transform and updates the frequency data.
/// </summary>
void fft_analyzer_t::Transform() noexcept
{
    double Norm = 0.;

    // Fill the FFT buffer from the sample ring buffer with Time domain data, apply the windowing function and determine the norm.
    {
        size_t i = (_Next - _FFTSize) % _InputRing.size();
        size_t j = 0;

        const audio_sample * const p = _InputRing.data();

        for (auto & Iter : _TimeData)
        {
            const double WindowFactor = _WindowFunction(msc::Map(j, (size_t) 0, _FFTSize - 1, -1., 1.));

            Iter = std::complex<double>(p[i] * WindowFactor, 0.);

            i = (i + 1) % _InputRing.size();

            Norm += WindowFactor;
            j++;
        }
    }

    // Normalize the Time domain data.
    {
        const double Factor = (double) _FFTSize / Norm; // * M_SQRT2;

        std::transform(std::execution::par_unseq, _TimeData.begin(), _TimeData.end(), _TimeData.begin(), [Factor](std::complex<double> x)
        {
            return x * Factor;
        });
    }

    // Transform the data from the Time domain to the Frequency domain.
    _FFT.Transform(_TimeData, _FreqData);

    // Normalize the Frequency domain data.
    {
        const double Factor = 2. / (double) _FFTSize;

        std::transform(std::execution::par_unseq, _FreqData.begin(), _FreqData.end(), _FreqData.begin(), [Factor](std::complex<double> x)
        {
            return x * Factor;
        });
    }
}

/// <summary>
/// Maps the Fast Fourier Transform coefficients on the frequency bands.
/// </summary>
void fft_analyzer_t::AnalyzeSamples(uint32_t sampleRate, frequency_bands_t & freqBands) const noexcept
{
    const bool IsRMS       =  (_State->_AggregationMethod == AggregationMethod::RMS || _State->_AggregationMethod == AggregationMethod::RMSSum);
    const bool IsMedian    =   _State->_AggregationMethod == AggregationMethod::Median;
    const bool UseBandGain =  (_State->_SmoothGainTransition && (_State->_AggregationMethod == AggregationMethod::Sum || _State->_AggregationMethod == AggregationMethod::RMSSum));
    const bool IsAverage   = ((_State->_AggregationMethod == AggregationMethod::Average || _State->_AggregationMethod == AggregationMethod::RMS) || UseBandGain);

    std::vector<double> Values;

    for (frequency_band_t & fb : freqBands)
    {
        const double BandGain = UseBandGain ? std::hypot(1, std::pow(((fb.Hi - fb.Lo) * (double) (_FreqData.size() - 1) / (double) sampleRate), (IsRMS ? 0.5 : 1.))) : 1.;

        double LoIdx = HzToBinIndex(fb.Lo, _FreqData.size(), sampleRate);
        double HiIdx = HzToBinIndex(fb.Hi, _FreqData.size(), sampleRate);

        LoIdx = (_State->_SmoothLowerFrequencies ? std::round(LoIdx) + 1. : std::ceil(LoIdx));
        HiIdx = (_State->_SmoothLowerFrequencies ? std::round(HiIdx) - 1. : std::floor(HiIdx));

        if (LoIdx <= HiIdx)
        {
            HiIdx -= std::max(HiIdx - LoIdx - (double) _FreqData.size(), 0.);

            double Value = (_State->_AggregationMethod == AggregationMethod::Minimum) ? DBL_MAX : 0.;

            Values.clear();

            int Count = 0;

            for (auto Idx = LoIdx; Idx <= HiIdx; ++Idx)
            {
                const size_t BinIdx = msc::Wrap((size_t) Idx, _FreqData.size());

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
            const double Index = HzToBinIndex(fb.Center, _FreqData.size(), sampleRate);

            fb.RawValue = std::fabs(Interpolate(_FreqData, Index, _State->_KernelSize)) * BandGain;
        }
    }
}

/// <summary>
/// Maps the Fast Fourier Transform coefficients on the frequency bands (Mel-Frequency Cepstrum, MFC).
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Mel-frequency_cepstrum</ref>
void fft_analyzer_t::AnalyzeSamplesUsingTFB(uint32_t sampleRate, frequency_bands_t & freqBands) const noexcept
{
    const double Scale = (double) _FreqData.size() / sampleRate;

    for (frequency_band_t & fb : freqBands)
    {
        double Sum = 0.;

        const double MinBin = std::min(fb.Lo, fb.Hi) * Scale;
        const double MidBin = fb.Center              * Scale;
        const double MaxBin = std::max(fb.Lo, fb.Hi) * Scale;

        const double OverflowCompensation = std::max(0., MaxBin - MinBin - (double) _FreqData.size());

        for (double i = std::floor(MidBin); i >= std::floor(MinBin + OverflowCompensation); --i)
            Sum += std::pow(std::abs(_FreqData[msc::Wrap((size_t) i, _FreqData.size())]) * std::max(msc::Map(i, MinBin, MidBin, 0., 1.), 0.), 2.);

        for (double i = std::ceil(MidBin); i <= std::ceil(MaxBin - OverflowCompensation); ++i)
            Sum += std::pow(std::abs(_FreqData[msc::Wrap((size_t) i, _FreqData.size())]) * std::max(msc::Map(i, MaxBin, MidBin, 0., 1.), 0.), 2.);

        fb.RawValue = std::sqrt(Sum);
    }
}

/// <summary>
/// Maps the Fast Fourier Transform coefficients on the frequency bands (Brown-Puckette).
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Pitch_detection_algorithm</ref>
void fft_analyzer_t::AnalyzeSamplesUsingBP(uint32_t sampleRate, frequency_bands_t & freqBands) const noexcept
{
    const double HzToBin = (double) _FreqData.size() / sampleRate;

    for (frequency_band_t & fb : freqBands)
    {
        double re = 0.;
        double im = 0.;

        const double Center      = fb.Center * HzToBin;

        const double Bandwidth    = std::abs(fb.Hi - fb.Lo) + (double) sampleRate / (double) _FreqData.size() * _State->_BandwidthOffset;
        const double tlen         = std::min(1. / Bandwidth, HzToBin / _State->_BandwidthCap);
        const double actualLength = _State->_UseGranularBandwidth ? tlen * sampleRate : std::min(std::trunc(std::pow(2., std::round(std::log2(tlen * sampleRate)))), (double) _FreqData.size() / _State->_BandwidthCap);
        const double flen         = std::min(_State->_BandwidthAmount * (double) _FreqData.size() / actualLength, (double) _FreqData.size());

        const double Start        = std::ceil (Center - flen / 2.);
        const double End          = std::floor(Center + flen / 2.);

        if (std::isfinite(Start) && std::isfinite(End))
        {
            for (int32_t i = (int32_t ) Start; i <= (int32_t ) End; ++i)
            {
                const double Sign = i & 1 ? -1. : 1.;
                const double posX = 2. * ((double) i - Center) / flen;
                const double w = _BrownPucketteKernel(posX);
                const double u = w * Sign;

                size_t idx = ((i % _FreqData.size()) + _FreqData.size()) % _FreqData.size();

                re += _FreqData[idx].real() * u;
                im += _FreqData[idx].imag() * u;
            }
        }

        fb.RawValue = std::hypot(re, im);
    }
}

/// <summary>
/// Uses a Lanczos kernel to determine the interpolated magnitude at a fractional index.
/// </summary>
double fft_analyzer_t::Interpolate(const std::vector<std::complex<double>> & fftCoeffs, double index, int kernelSize) const noexcept
{
    std::complex<double> Sum = 0.;

    for (int i = -kernelSize + 1; i <= kernelSize; ++i)
    {
        const double Index = std::floor(index) + i;
        const double x = (index - Index) * M_PI;

        double Weight = (std::fabs(x) > 0.) ? std::sin(x) / (x) * std::sin(x / kernelSize) / (x / kernelSize) : 0.;

        // Flip the sign of the weight for even indexes (non-standard for Lanczos).

        if ((i & 1) == 0)
            Weight = -Weight;

        const size_t CoefIdx = msc::Wrap((size_t) Index, fftCoeffs.size());

        Sum += fftCoeffs[CoefIdx] * Weight;
    }

    return std::abs(Sum);
}

/// <summary>
/// Calculates the median.
/// </summary>
double fft_analyzer_t::Median(std::vector<double> & data) const noexcept
{
    if (data.empty())
        return DBL_MIN;

    if (data.size() == 1)
        return data[0];

    std::sort(data.begin(), data.end());

    const size_t Mid = data.size() / 2;

    return (data.size() % 2) ? data[Mid] : (data[Mid - 1] + data[Mid]) / 2.;
}


/** $VER: FFTAnalyzer.cpp (2026.10.28) P. Stuer **/

#include "pch.h"

#include "FFTAnalyzer.h"

#include "Aggregator.h"

#pragma hdrstop

/// <summary>
/// Initializes an instance of the class.
/// </summary>
fft_analyzer_t::fft_analyzer_t(const state_t & state, size_t fftSize, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction, const window_function_t & brownPucketteKernel) noexcept : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction), _BrownPucketteKernel(brownPucketteKernel)
{
    _FFTSize = fftSize;

    _FFT.Initialize(_FFTSize);

    // Create the input ring buffer.
    _InputRing.resize(_FFTSize, (audio_sample) 0.);
    _Next = 0;

    _Samples.resize(_FFTSize);
    _Coefficients.resize(_FFTSize);
}

/// <summary>
/// Calculates the transform and returns the frequency bands.
/// </summary>
bool fft_analyzer_t::AnalyzeAudio(const audio_sample * frameData, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept
{
//  const auto Start = std::chrono::steady_clock::now();

    Add(frameData, frameCount, selectedChannels);

    Transform();

    switch (_State._MappingMethod)
    {
        default:

        case CoefficientMapping::Standard:
        {
            MapCoefficients(frequencyBands);
            break;
        }

        case CoefficientMapping::TriangularFilterBank:
        {
            MapCoefficientsUsingTFB(frequencyBands);
            break;
        }

        case CoefficientMapping::BrownPuckette:
        {
            MapCoefficientsUsingBP(frequencyBands);
            break;
        }

        case CoefficientMapping::GaussianFilter:
        {
            MapCoefficientsUsingGaussianFilter(frequencyBands);
            break;
        }
    }

//  const auto Finish = std::chrono::steady_clock::now();

//  Log.AtInfo().Write("%6d ms", std::chrono::duration_cast<std::chrono::microseconds>(Finish - Start));

    return true;
}

/// <summary>
/// Adds multiple samples to the input buffer.
/// </summary>
void fft_analyzer_t::Add(const audio_sample * samples, size_t frameCount, uint32_t selectedChannels) noexcept
{
    if (samples == nullptr)
        return;

    const size_t SampleCount = frameCount * _ChannelCount;

    audio_sample * const Mono = _InputRing.data();

    const auto Size = _InputRing.size();

    for (size_t i = 0; i < SampleCount; i += _ChannelCount)
    {
        // Downmix the selected channels to mono before adding the sample to the input ring buffer.
        Mono[_Next] = Downmix(&samples[i], selectedChannels);

        ++_Next;

        if (_Next == Size)
            _Next = 0;
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
        const auto Size = _InputRing.size();

        size_t i = _Next;
        size_t j = 0;

        const audio_sample * const Sample = _InputRing.data();

        for (auto & Iter : _Samples)
        {
            const double WindowFactor = WindowFunction(msc::Map(j, (size_t) 0, _FFTSize - 1, -1., 1.));

            Iter = std::complex<double>(Sample[i] * WindowFactor, 0.);

            Norm += WindowFactor;

            ++i;

            if (i == Size)
                i = 0;

            ++j;
        }
    }

    // Make sure the norm is acceptable.
    const double MinimumNorm = std::numeric_limits<double>::epsilon() * (double) _FFTSize;

    if (std::abs(Norm) <= MinimumNorm)
    {
        std::fill(_Coefficients.begin(), _Coefficients.end(), std::complex<double>{});

        return;
    }

    // Normalize the samples (Time domain data).
    {
        const double Factor = (double) _FFTSize / Norm;

        for (auto & Sample : _Samples)
            Sample *= Factor;
    }

    // Transform the data from the Time domain to the Frequency domain.
    _FFT.Transform(_Samples, _Coefficients);

    // Normalize the coefficients (Frequency domain data).
    {
        const double Factor = 2. / (double) _FFTSize;

        for (auto & Coefficient : _Coefficients)
            Coefficient *= Factor;
    }

    // Correct the DC and the Nyquist bins because they do not have a mirror frequency. Effectively normalization factor 1/N.
    _Coefficients.front() *= 0.5;           // DC bin

    if ((_FFTSize % 2) == 0)
        _Coefficients[_FFTSize / 2] *= 0.5; // Nyquist bin
}

/// <summary>
/// Maps complex FFT coefficients onto frequency-band magnitudes.
/// </summary>
void fft_analyzer_t::MapCoefficients(frequency_bands_t & freqBands) noexcept
{
    if (_Coefficients.empty() || (_SampleRate == 0))
    {
        for (frequency_band_t & fb : freqBands)
            fb.Magnitude = 0.;

        return;
    }

    const auto AggregationMethod      = _State._AggregationMethod;
    const bool SmoothLowerFrequencies = _State._SmoothLowerFrequencies;
    const bool SmoothGainTransition   = _State._SmoothGainTransition && (AggregationMethod == AggregationMethod::Sum || AggregationMethod == AggregationMethod::RMSSum);
    const auto KernelSize             = _State._KernelSize;

    const bool IsRMS = (AggregationMethod == AggregationMethod::RMS) || (AggregationMethod == AggregationMethod::RMSSum);

    const auto MaxPositiveBinIndex = _FFTSize / 2; // Corresponds to the Nyquist bin for even FFT sizes.
    const auto NyquistBinIndex     = (double) _FFTSize / 2.;

    // Create the aggregator. It returns the aggregate magnitude of the specified bins.
    auto Aggregate = aggregator_t::Create(AggregationMethod, _FFTSize, SmoothGainTransition);

    // Determine the aggregated magnitude for each frequency band.
    for (frequency_band_t & fb : freqBands)
    {
        assert((fb.Low <= fb.Center) && (fb.Center <= fb.High));

        const auto LoBin = HzToBinIndex(fb.Low);
        const auto HiBin = HzToBinIndex(fb.High);

        double BandGain = 1.;

        if (SmoothGainTransition)
        {
            const double WidthInBins = std::max(HiBin - LoBin, 0.);

            BandGain = IsRMS ? std::sqrt(1. + WidthInBins) : std::hypot(1., WidthInBins);
        }

        // Determine the first and last complete bin to aggregate.
        auto FirstBin = (ptrdiff_t) (SmoothLowerFrequencies ? std::round(LoBin) + 1. : std::ceil (LoBin));
        auto LastBin  = (ptrdiff_t) (SmoothLowerFrequencies ? std::round(HiBin) - 1. : std::floor(HiBin));

        FirstBin = std::clamp(FirstBin, (ptrdiff_t) 0, (ptrdiff_t) (MaxPositiveBinIndex));
        LastBin  = std::clamp(LastBin,  (ptrdiff_t) 0, (ptrdiff_t) (MaxPositiveBinIndex));

        if (FirstBin <= LastBin)
        {
            const auto RequestedBins = (size_t) (LastBin - FirstBin + 1);

            const double Magnitude = (*Aggregate)(_Coefficients, RequestedBins, (size_t) FirstBin, _FFTSize);

            fb.Magnitude = Magnitude *  BandGain;
        }
        else
        {
            const double BinIndex = HzToBinIndex(fb.Center);

            if ((BinIndex >= 0.) && (BinIndex <= (double) NyquistBinIndex))
                fb.Magnitude = Interpolate(_Coefficients, BinIndex, KernelSize) * BandGain;
            else
                fb.Magnitude = 0.;
        }
    }
}

/// <summary>
/// Maps FFT power-spectrum coefficients using a triangular filter bank. Assumes the frequency bands are spaced on the Mel scale.
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Mel-frequency_cepstrum</ref>
void fft_analyzer_t::MapCoefficientsUsingTFB(frequency_bands_t & freqBands) const noexcept
{
    if (_Coefficients.empty() || (_SampleRate == 0))
    {
        for (frequency_band_t & fb : freqBands)
            fb.Magnitude = 0.;

        return;
    }

    const double NyquistBinIndex = (double) (_FFTSize / 2);

    for (frequency_band_t& fb : freqBands)
    {
        assert(fb.Low    <= fb.Center);
        assert(fb.Center <= fb.High);

        fb.Magnitude = 0.;

        if (fb.High < 0. || fb.Low > NyquistFrequency)
            continue;

        const double LoBin  = std::clamp(HzToBinIndex(fb.Low),    0.,    NyquistBinIndex);
        const double MidBin = std::clamp(HzToBinIndex(fb.Center), LoBin, NyquistBinIndex);

        const double HiBin = std::clamp(HzToBinIndex(fb.High), MidBin, NyquistBinIndex);

        if (!(LoBin < MidBin && MidBin < HiBin))
        {
            fb.Magnitude = Interpolate(_Coefficients, MidBin, _State._KernelSize);
            continue;
        }

        const size_t StartBin = (size_t) std::ceil(LoBin);
        const size_t EndBin   = (size_t) std::floor(HiBin);

        if (StartBin > EndBin)
            continue;

        double WeightedPower = 0.;

        for (size_t Bin = StartBin; Bin <= EndBin; ++Bin)
        {
            const auto Index = (double) Bin;

            const double Weight = (Index <= MidBin) ? (Index - LoBin) / (MidBin - LoBin) : (HiBin - Index) / (HiBin - MidBin);

            // Contribute the power of the bin weighed by the triangular filter bank.
            WeightedPower += std::norm(_Coefficients[Bin]) * std::clamp(Weight, 0., 1.);
        }

        fb.Magnitude = std::sqrt(std::max(WeightedPower, 0.));
    }
}

/// <summary>
/// Maps FFT power-spectrum coefficients onto the frequency bands. (Brown-Puckette)
/// <ref>https://en.wikipedia.org/wiki/Pitch_detection_algorithm</ref>
void fft_analyzer_t::MapCoefficientsUsingBP(frequency_bands_t & freqBands) const noexcept
{
    if (_Coefficients.empty() || (_SampleRate == 0) || (_State._BandwidthCap <= 0.) || (_State._BandwidthAmount <= 0.))
    {
        for (frequency_band_t & fb : freqBands)
            fb.Magnitude = 0.;

        return;
    }

    const double FFTSize = (double) _FFTSize;
    const double HzToBin = FFTSize / (double) _SampleRate;
    const double BinToHz = (double) _SampleRate / FFTSize; // Frequency resolution

    for (frequency_band_t & fb : freqBands)
    {
        fb.Magnitude = 0.;

        if (fb.Center < 0. || fb.Center > NyquistFrequency)
            continue;

        double re = 0.;
        double im = 0.;

        double Norm = 0.;

        // In Hz
        const double Bandwidth      = std::max(std::abs(fb.High - fb.Low) + (BinToHz * _State._BandwidthOffset), std::numeric_limits<double>::epsilon());
        // In seconds (Time domain)
        const double WindowDuration = std::min(1. / Bandwidth, HzToBin / _State._BandwidthCap);
        // In samples (Sample domain)
        const double WindowSize     = std::max(_State._UseGranularBandwidth ? WindowDuration * _SampleRate : std::min(std::trunc(std::pow(2., std::round(std::log2(WindowDuration * _SampleRate)))), FFTSize / _State._BandwidthCap), 1.);
        // In bins (Frequency domain)
        const double KernelSize     = std::clamp(_State._BandwidthAmount * FFTSize / WindowSize, 1., FFTSize);

        const double Center = fb.Center * HzToBin;

        const double Half = 0.5 * (KernelSize - 1.);

        const auto Start = (int64_t) std::ceil (Center - Half);
        const auto End   = (int64_t) std::floor(Center + Half);

        for (int64_t i = Start; i <= End; ++i)
        {
            const double NormalizedBinOffset = 2. * ((double) i - Center) / KernelSize; // Within [-1, 1]

            double Weight = _BrownPucketteKernel(NormalizedBinOffset);

            // Improves low-frequency spectral reconstruction by alternating the phase of neighboring interpolation taps. Non-standard, visualization-specific adjustment.
            if ((i & 1) == 0)
                Weight = -Weight;

            Norm += Weight * Weight;

            const auto Index = (size_t) msc::Wrap(i, (int64_t) _FFTSize);

            re += _Coefficients[Index].real() * Weight;
            im += _Coefficients[Index].imag() * Weight;
        }

        if (Norm > 0.)
        {
            const double InvNorm = 1. / std::sqrt(Norm);

            re *= InvNorm;
            im *= InvNorm;
        }

        fb.Magnitude = std::hypot(re, im);
    }
}

/// <summary>
/// Maps FFT power-spectrum coefficients into logarithmic frequency bands using Gaussian frequency-domain weighting. (Continuous Wavelet Transform approximation)
/// </summary>
void fft_analyzer_t::MapCoefficientsUsingGaussianFilter(frequency_bands_t & freqBands) const noexcept
{
    if (_Coefficients.empty() || !(_SampleRate > 0.))
    {
        for (frequency_band_t & fb : freqBands)
            fb.Magnitude = 0.;

        return;
    }

    const double FFTSize         = (double) _FFTSize;
    const double NyquistBinIndex = FFTSize / 2;
    const double BinToHz         = (double) _SampleRate / FFTSize;

    // Map the linear FFT bins into logarithmically spaced "scales".
    for (frequency_band_t & fb : freqBands)
    {
        assert(fb.Low < fb.High);

        fb.Magnitude = 0.;

        if (fb.High < 0. || fb.Low > NyquistFrequency)
            continue;

        const double BandWidth = 4. / (fb.High - fb.Low);

        // Determine which FFT bins will contribute.
        const double LoBinValue = std::clamp(std::floor(HzToBinIndex(fb.Low)), 0., NyquistBinIndex);
        const double HiBinValue = std::clamp(std::ceil(HzToBinIndex(fb.High)), 0., NyquistBinIndex);

        const auto LoBin = (size_t) LoBinValue;
        const auto HiBin = (size_t) HiBinValue;

        if (LoBin > HiBin)
            continue;

        double Sum = 0.;
        double WeightSum = 0.;

        for (size_t i = LoBin; i <= HiBin; ++i)
        {
            const double Frequency = (double) i * BinToHz;

            // Apply Gaussian weighting. Bins near the center frequency get a higher weight.
            const double Distance = (Frequency - fb.Center) * BandWidth;
            const double Weight   = std::exp(-0.5 * Distance * Distance);

            // Compute the weighted average.
            Sum       += std::abs(_Coefficients[i]) * Weight;
            WeightSum += Weight;
        }

        if (WeightSum > 0.)
            fb.Magnitude = Sum / WeightSum;
    }
}

/// <summary>
/// Determines the interpolated magnitude at a fractional index.
/// </summary>
double fft_analyzer_t::Interpolate(const std::vector<std::complex<double>> & fftCoeffs, const double index, const int kernelSize) const noexcept
{
    if (fftCoeffs.empty() || kernelSize <= 0 || !std::isfinite(index))
        return 0.;

    const auto CoefficientCount = fftCoeffs.size();
    const auto Base = (int64_t) std::floor(index);

    // index = base + fraction, where fraction is in [0, 1).
    const double Fraction          = index - (double) Base;
    const double InverseKernelSize = 1. / (double) kernelSize;

    const auto sinc = [](const double x) noexcept
    {
        return (x == 0.) ? 1. : std::sin(x) / x;
    };

    std::complex<double> Sum{};

    // Wrap only once before entering the loop.
    int64_t WrappedIndex = (Base - kernelSize) % (int64_t) CoefficientCount;

    if (WrappedIndex < 0)
        WrappedIndex += (int64_t) CoefficientCount;

    auto CoefficientIndex = (size_t) WrappedIndex;

    for (int Tap = -kernelSize; Tap <= kernelSize; ++Tap)
    {
        // Distance from the interpolation point.
        const double Distance = (Fraction - (double) Tap) * Pi;

        // Lanczos-a kernel.
        double Weight = sinc(Distance) * sinc(Distance * InverseKernelSize);

        // Preserve the existing visualization-specific phase adjustment.
        if ((Tap & 1) == 0)
            Weight = -Weight;

        Sum += fftCoeffs[CoefficientIndex] * Weight;

        if (++CoefficientIndex == CoefficientCount)
            CoefficientIndex = 0;
    }

    return std::abs(Sum);
}

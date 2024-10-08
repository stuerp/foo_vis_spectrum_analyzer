
/** $VER: FFTAnalyzer.cpp (2024.02.17) P. Stuer - Based on TF3RDL's FFT analyzer, https://codepen.io/TF3RDL/pen/poQJwRW **/

#include "framework.h"
#include "FFTAnalyzer.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Destroys this instance.
/// </summary>
FFTAnalyzer::~FFTAnalyzer()
{
    if (_Data)
    {
        delete[] _Data;
        _Data = nullptr;
    }
}

/// <summary>
/// Initializes an instance of the class.
/// </summary>
FFTAnalyzer::FFTAnalyzer(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction, const WindowFunction & brownPucketteKernel, size_t fftSize) : Analyzer(state, sampleRate, channelCount, channelSetup, windowFunction), _BrownPucketteKernel(brownPucketteKernel)
{
    _FFTSize = fftSize;

    _FFT.Initialize(_FFTSize);

    // Create the ring buffer for the samples.
    _Size = _FFTSize;
    _Data = new audio_sample[_Size];

    ::memset(_Data, 0, sizeof(audio_sample) * _Size);

    _Curr = 0;

    _TimeData.resize(_FFTSize);
    _FreqData.resize(_FFTSize);
}

/// <summary>
/// Calculates the transform and returns the frequency bands.
/// </summary>
bool FFTAnalyzer::AnalyzeSamples(const audio_sample * samples, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept
{
    Add(samples, sampleCount, channels);

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

    return true;
}

/// <summary>
/// Adds multiple samples to the analyzer buffer.
/// It assumes that the buffer contains tuples of sample data for each channel. E.g. for 2 channels: Left(0), Right(0), Left(1), Right(1) ... Left(n), Right(n)
/// </summary>
void FFTAnalyzer::Add(const audio_sample * samples, size_t sampleCount, uint32_t channels) noexcept
{
//  Log::Write(Log::Level::Trace, "%8d: %5d, %5d, %5d", (int) ::GetTickCount64() _Size, _Curr, sampleCount / 2);

    if (samples == nullptr)
        return;

    // Make sure there are enough samples for all the channels.
    sampleCount -= (sampleCount % _ChannelCount);

    // Merge the samples of all channels into one averaged sample.
    #pragma loop(hint_parallel(2))
    for (size_t i = 0; i < sampleCount; i += _ChannelCount)
    {
        _Data[_Curr] = AverageSamples(&samples[i], channels);

        _Curr = (_Curr + 1) % _Size;
    }
}

/// <summary>
/// Calculates the Fast Fourier Transform and updates the frequency data.
/// </summary>
void FFTAnalyzer::Transform() noexcept
{
    double Norm = 0.;

    // Fill the FFT buffer from the sample ring buffer with Time domain data and apply the windowing function.
    {
        size_t i = _Curr;
        size_t j = 0;

        for (complex<double> & Iter : _TimeData)
        {
            double WindowFactor = _WindowFunction(Map(j, (size_t) 0, _FFTSize, -1., 1.));

            Iter = complex<double>(_Data[i] * WindowFactor, 0.);

            i = (i + 1) % _Size;

            Norm += WindowFactor;
            j++;
        }
    }

    // Normalize the Time domain data.
    {
        const double Factor = (double) _FFTSize / Norm / M_SQRT2;

        for (complex<double> & Iter : _TimeData)
            Iter *= Factor;
    }

    // Transform the data from the Time domain to the Frequency domain.
    _FFT.Transform(_TimeData, _FreqData);

    // Normalize the Frequency domain data.
    for (complex<double> & Iter : _FreqData)
        Iter /= (double) _FFTSize / 2.;
}

/// <summary>
/// Maps the Fast Fourier Transform coefficients on the frequency bands.
/// </summary>
void FFTAnalyzer::AnalyzeSamples(uint32_t sampleRate, FrequencyBands & freqBands) const noexcept
{
    const bool UseBandGain = (_State->_SmoothGainTransition && (_State->_SummationMethod == SummationMethod::Sum || _State->_SummationMethod == SummationMethod::RMSSum));
    const bool IsRMS = (_State->_SummationMethod == SummationMethod::RMS || _State->_SummationMethod == SummationMethod::RMSSum);

    std::vector<double> Values(16);

    for (FrequencyBand & fb : freqBands)
    {
        const double LoHz = HzToFFTIndex(Min(fb.Hi, fb.Lo), _FreqData.size(), sampleRate);
        const double HiHz = HzToFFTIndex(Max(fb.Hi, fb.Lo), _FreqData.size(), sampleRate);

        const int LoIdx = (int) (_State->_SmoothLowerFrequencies ? ::round(LoHz) + 1. : ::ceil(LoHz));
              int HiIdx = (int) (_State->_SmoothLowerFrequencies ? ::round(HiHz) - 1. : ::floor(HiHz));

        const double BandGain =  UseBandGain ? ::hypot(1, ::pow(((fb.Hi - fb.Lo) * (double) _FreqData.size() / (double) sampleRate), (IsRMS ? 0.5 : 1.))) : 1.;

        if (LoIdx <= HiIdx)
        {
            HiIdx -= Max(HiIdx - LoIdx - (int) _FreqData.size(), 0);

            double Value = (_State->_SummationMethod == SummationMethod::Minimum) ? DBL_MAX : 0.;

            Values.clear();

            int Count = 0;

            for (int Idx = LoIdx; Idx <= HiIdx; ++Idx)
            {
                size_t CoefIdx = Wrap((size_t) Idx, _FreqData.size());

                double Coef = std::abs(_FreqData[CoefIdx]);

                switch (_State->_SummationMethod)
                {
                    case SummationMethod::Minimum:
                        Value = Min(Coef, Value);
                        break;

                    case SummationMethod::Maximum:
                        Value = Max(Coef, Value);
                        break;

                    case SummationMethod::Sum:
                    case SummationMethod::Average:
                        Value += Coef;
                        break;

                    case SummationMethod::RMS:
                    case SummationMethod::RMSSum:
                        Value += Coef * Coef;
                        break;

                    case SummationMethod::Median:
                        Values.push_back(Coef);
                        break;

                    default:
                        Value = Coef;
                }

                ++Count;
            }

            if (((_State->_SummationMethod == SummationMethod::Average || _State->_SummationMethod == SummationMethod::RMS) || UseBandGain) && (Count != 0))
                Value /= Count;
            else
            if (_State->_SummationMethod == SummationMethod::Median)
                Value = Median(Values);

            fb.NewValue = (IsRMS ? ::sqrt(Value) : Value) * BandGain;
        }
        else
        {
            const double Value = fb.Ctr * (double) _FreqData.size() / sampleRate;

            fb.NewValue = ::fabs(Lanzcos(_FreqData, Value, _State->_KernelSize)) * BandGain;
        }
    }
}

/// <summary>
/// Maps the Fast Fourier Transform coefficients on the frequency bands (Mel-Frequency Cepstrum, MFC).
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Mel-frequency_cepstrum</ref>
void FFTAnalyzer::AnalyzeSamplesUsingTFB(uint32_t sampleRate, FrequencyBands & freqBands) const noexcept
{
    for (FrequencyBand & fb : freqBands)
    {
        double Sum = 0.;

        const double MinBin = Min(fb.Lo, fb.Hi) * (double) _FreqData.size() / sampleRate;
        const double MidBin = fb.Ctr            * (double) _FreqData.size() / sampleRate;
        const double MaxBin = Max(fb.Lo, fb.Hi) * (double) _FreqData.size() / sampleRate;

        const double OverflowCompensation = Max(0., MaxBin - MinBin - (double) _FreqData.size());

        for (double i = ::floor(MidBin); i >= ::floor(MinBin + OverflowCompensation); --i)
            Sum += ::pow(std::abs(_FreqData[Wrap((size_t) i, _FreqData.size())]) * Max(Map(i, MinBin, MidBin, 0., 1.), 0.), 2.);

        for (double i = ::ceil(MidBin); i <= ::ceil(MaxBin - OverflowCompensation); ++i)
            Sum += ::pow(std::abs(_FreqData[Wrap((size_t) i, _FreqData.size())]) * Max(Map(i, MaxBin, MidBin, 0., 1.), 0.), 2.);

        fb.NewValue = ::sqrt(Sum);
    }
}

/// <summary>
/// Maps the Fast Fourier Transform coefficients on the frequency bands (Brown-Puckette).
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Pitch_detection_algorithm</ref>
void FFTAnalyzer::AnalyzeSamplesUsingBP(uint32_t sampleRate, FrequencyBands & freqBands) const noexcept
{
    const double HzToBin = (double) _FreqData.size() / sampleRate;

    for (FrequencyBand & fb : freqBands)
    {
        double re = 0.;
        double im = 0.;

        const double Center      = fb.Ctr * HzToBin;

        const double Bandwidth    = ::abs(fb.Hi - fb.Lo) + (double) sampleRate / (double) _FreqData.size() * _State->_BandwidthOffset;
        const double tlen         = Min(1. / Bandwidth, HzToBin / _State->_BandwidthCap);
        const double actualLength = _State->_UseGranularBandwidth ? tlen * sampleRate : Min(::trunc(::pow(2., ::round(::log2(tlen * sampleRate)))), (double) _FreqData.size() / _State->_BandwidthCap);
        const double flen         = Min(_State->_BandwidthAmount * (double) _FreqData.size() / actualLength, (double) _FreqData.size());

        const double Start        = ::ceil (Center - flen / 2.);
        const double End          = ::floor(Center + flen / 2.);

        if (::isfinite(Start) && ::isfinite(End))
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

        fb.NewValue = ::hypot(re, im);
    }
}

/// <summary>
/// Applies a Lanzcos kernel to the specified value.
/// </summary>
double FFTAnalyzer::Lanzcos(const std::vector<complex<double>> & fftCoeffs, double value, int kernelSize) const noexcept
{
    double re = 0.;
    double im = 0.;

    for (int i = -kernelSize + 1; i <= kernelSize; ++i)
    {
        double Pos = ::floor(value) + i;
        double Twiddle = value - Pos; // -Pos + ::round(Pos) + i

        double w = (::fabs(Twiddle) <= 0.) ? 1. : ::sin(Twiddle * M_PI) / (Twiddle * M_PI) * ::sin(M_PI * Twiddle / kernelSize) / (M_PI * Twiddle / kernelSize);

        size_t CoefIdx = Wrap((size_t) Pos, fftCoeffs.size() / 2);

        re += fftCoeffs[CoefIdx].real() * w * (-1 + Wrap(i, 2) * 2);
        im += fftCoeffs[CoefIdx].imag() * w * (-1 + Wrap(i, 2) * 2);
    }

    return ::hypot(re, im);
}

/// <summary>
/// Calculates the median.
/// </summary>
double FFTAnalyzer::Median(std::vector<double> & data) const noexcept
{
    if (data.size() == 0)
        return DBL_MIN;

    if (data.size() == 1)
        return data[0];

    std::sort(data.begin(), data.end());

    size_t Mid = data.size() / 2;

    return (data.size() % 2) ? data[Mid] : (data[Mid - 1] + data[Mid]) / 2.;
}

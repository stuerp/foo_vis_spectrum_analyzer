
/** $VER: FFTAnalyzer.cpp (2024.01.02) P. Stuer **/

#include "FFTAnalyzer.h"

#include "Log.h"

#include <algorithm>

#pragma hdrstop

/// <summary>
/// Initializes an instance of the class.
/// </summary>
FFTAnalyzer::FFTAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, size_t fftSize, const Configuration * configuration) : FFTProvider(channelCount, channelSetup, sampleRate, windowFunction, fftSize)
{
    _Configuration = configuration;
}

/// <summary>
/// Maps Fast Fourier Transform coefficients on the frequency bands.
/// </summary>
void FFTAnalyzer::GetFrequencyBands(const std::vector<std::complex<double>> & coefficients, uint32_t sampleRate, SummationMethod summationMethod, std::vector<FrequencyBand> & freqBands) const noexcept
{
    const bool UseBandGain = (_Configuration->_SmoothGainTransition && (summationMethod == SummationMethod::Sum || summationMethod == SummationMethod::RMSSum));
    const bool IsRMS = (summationMethod == SummationMethod::RMS || summationMethod == SummationMethod::RMSSum);

    std::vector<double> Values(16);

    for (FrequencyBand & Iter : freqBands)
    {
        const double LoHz = HzToFFTIndex(Min(Iter.Hi, Iter.Lo), coefficients.size(), sampleRate);
        const double HiHz = HzToFFTIndex(Max(Iter.Hi, Iter.Lo), coefficients.size(), sampleRate);

        const int LoIdx = (int) (_Configuration->_SmoothLowerFrequencies ? ::round(LoHz) + 1. : ::ceil(LoHz));
                int HiIdx = (int) (_Configuration->_SmoothLowerFrequencies ? ::round(HiHz) - 1. : ::floor(HiHz));

        const double BandGain =  UseBandGain ? ::hypot(1, ::pow(((Iter.Hi - Iter.Lo) * (double) coefficients.size() / (double) sampleRate), (IsRMS ? 0.5 : 1.))) : 1.;

        if (LoIdx <= HiIdx)
        {
            HiIdx -= Max(HiIdx - LoIdx - (int) coefficients.size(), 0);

            double Value = (summationMethod == SummationMethod::Minimum) ? DBL_MAX : 0.;

            Values.clear();

            int Count = 0;

            for (int Idx = LoIdx; Idx <= HiIdx; ++Idx)
            {
                size_t CoefIdx = Wrap((size_t) Idx, coefficients.size());

                double Coef = std::abs(coefficients[CoefIdx]);

                switch (summationMethod)
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

            if (((summationMethod == SummationMethod::Average || summationMethod == SummationMethod::RMS) || UseBandGain) && (Count != 0))
                Value /= Count;
            else
            if (summationMethod == SummationMethod::Median)
                Value = Median(Values);

            Iter.NewValue = (IsRMS ? ::sqrt(Value) : Value) * BandGain;
        }
        else
        {
            const double Value = Iter.Ctr * (double) coefficients.size() / sampleRate;

            Iter.NewValue = ::fabs(Lanzcos(coefficients, Value, _Configuration->_KernelSize)) * BandGain;
        }
    }
}

/// <summary>
/// Maps Fast Fourier Transform coefficients on the frequency bands (Mel-Frequency Cepstrum, MFC).
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Mel-frequency_cepstrum</ref>
void FFTAnalyzer::GetFrequencyBands(const std::vector<std::complex<double>> & coefficients, uint32_t sampleRate, std::vector<FrequencyBand> & freqBands) const noexcept
{
    for (FrequencyBand & Iter : freqBands)
    {
        double Sum = 0.;

        const double MinBin = Min(Iter.Lo, Iter.Hi) * (double) coefficients.size() / sampleRate;
        const double MidBin = Iter.Ctr              * (double) coefficients.size() / sampleRate;
        const double MaxBin = Max(Iter.Lo, Iter.Hi) * (double) coefficients.size() / sampleRate;

        const double OverflowCompensation = Max(0., MaxBin - MinBin - (double) coefficients.size());

        for (double i = ::floor(MidBin); i >= ::floor(MinBin + OverflowCompensation); --i)
            Sum += ::pow(std::abs(coefficients[Wrap((size_t) i, coefficients.size())]) * Max(Map(i, MinBin, MidBin, 0., 1.), 0.), 2.);

        for (double i = ::ceil(MidBin); i <= ::ceil(MaxBin - OverflowCompensation); ++i)
            Sum += ::pow(std::abs(coefficients[Wrap((size_t) i, coefficients.size())]) * Max(Map(i, MaxBin, MidBin, 0., 1.), 0.), 2.);

        Iter.NewValue = ::sqrt(Sum);
    }
}

/// <summary>
/// Maps Fast Fourier Transform coefficients on the frequency bands (Brown-Puckette).
/// </summary>
/// <ref>https://en.wikipedia.org/wiki/Pitch_detection_algorithm</ref>
void FFTAnalyzer::GetFrequencyBands(const std::vector<std::complex<double>> & coefficients, uint32_t sampleRate, const WindowFunction & windowFunction, double bandwidthOffset, double bandwidthCap, double bandwidthAmount, bool granularBW, std::vector<FrequencyBand> & freqBands) const noexcept
{
    const double HzToBin = (double) coefficients.size() / sampleRate;

    for (FrequencyBand & Iter : freqBands)
    {
        double re = 0.;
        double im = 0.;

        double Center      = Iter.Ctr * HzToBin;

        double bandwidth    = ::abs(Iter.Hi - Iter.Lo) + (double) sampleRate / (double) coefficients.size() * bandwidthOffset;
        double tlen         = Min(1. / bandwidth, HzToBin / bandwidthCap);
        double actualLength = granularBW ? tlen * sampleRate : Min(::trunc(::pow(2., ::round(::log2(tlen * sampleRate)))), (double) coefficients.size() / bandwidthCap);
        double flen         = Min(bandwidthAmount * (double) coefficients.size() / actualLength, (double) coefficients.size());

        double Start        = ::ceil (Center - flen / 2.);
        double End          = ::floor(Center + flen / 2.);

        if (::isfinite(Start) && ::isfinite(End))
        {
            for (int32_t i = (int32_t ) Start; i <= (int32_t ) End; ++i)
            {
                double Sign = i & 1 ? -1. : 1.;
                double posX = 2. * ((double) i - Center) / flen;
                double w = windowFunction(posX);
                double u = w * Sign;

                size_t idx = ((i % coefficients.size()) + coefficients.size()) % coefficients.size();

                re += coefficients[idx].real() * u;
                im += coefficients[idx].imag() * u;
            }
        }

        Iter.NewValue = ::hypot(re, im);
    }
}

/// <summary>
/// Calculates the position of the peak indicators.
/// </summary>
void FFTAnalyzer::UpdatePeakIndicators(std::vector<FrequencyBand> & frequencyBands) const noexcept
{
    for (FrequencyBand & Iter : frequencyBands)
    {
        double Amplitude = Clamp(_Configuration->ScaleA(Iter.CurValue), 0., 1.);

        if (Amplitude >= Iter.Peak)
        {
            if ((_Configuration->_PeakMode == PeakMode::AIMP) || (_Configuration->_PeakMode == PeakMode::FadingAIMP))
                Iter.HoldTime = (::isfinite(Iter.HoldTime) ? Iter.HoldTime : 0.) + (Amplitude - Iter.Peak) * _Configuration->_HoldTime;
            else
                Iter.HoldTime = _Configuration->_HoldTime;

            Iter.Peak = Amplitude;
            Iter.DecaySpeed = 0.;
            Iter.Opacity = 1.;
        }
        else
        {
            if (Iter.HoldTime >= 0.)
            {
                if ((_Configuration->_PeakMode == PeakMode::AIMP) || (_Configuration->_PeakMode == PeakMode::FadingAIMP))
                    Iter.Peak += (Iter.HoldTime - Max(Iter.HoldTime - 1., 0.)) / _Configuration->_HoldTime;

                Iter.HoldTime -= 1.;

                if ((_Configuration->_PeakMode == PeakMode::AIMP) || (_Configuration->_PeakMode == PeakMode::FadingAIMP))
                    Iter.HoldTime = Min(Iter.HoldTime, _Configuration->_HoldTime);
            }
            else
            {
                switch (_Configuration->_PeakMode)
                {
                    default:

                    case PeakMode::None:
                        break;

                    case PeakMode::Classic:
                        Iter.DecaySpeed = _Configuration->_Acceleration / 256.;
                        Iter.Peak -= Iter.DecaySpeed;
                        break;

                    case PeakMode::Gravity:
                        Iter.DecaySpeed += _Configuration->_Acceleration / 256.;
                        Iter.Peak -= Iter.DecaySpeed;
                        break;

                    case PeakMode::AIMP:
                        Iter.DecaySpeed = (_Configuration->_Acceleration / 256.) * (1. + (int) (Iter.Peak < 0.5));
                        Iter.Peak -= Iter.DecaySpeed;
                        break;

                    case PeakMode::FadeOut:
                        Iter.DecaySpeed += _Configuration->_Acceleration / 256.;
                        Iter.Opacity -= Iter.DecaySpeed;

                        if (Iter.Opacity <= 0.)
                            Iter.Peak = Amplitude;
                        break;

                    case PeakMode::FadingAIMP:
                        Iter.DecaySpeed = (_Configuration->_Acceleration / 256.) * (1. + (int) (Iter.Peak < 0.5));
                        Iter.Peak -= Iter.DecaySpeed;
                        Iter.Opacity -= Iter.DecaySpeed;

                        if (Iter.Opacity <= 0.)
                            Iter.Peak = Amplitude;
                        break;
                }
            }

            Iter.Peak = Clamp(Iter.Peak, 0., 1.);
        }
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

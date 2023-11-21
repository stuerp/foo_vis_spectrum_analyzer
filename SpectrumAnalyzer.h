
/** $VER: SpectrumAnalyzer.h (2023.11.21) P. Stuer **/

#include "framework.h"

#include "FrequencyBand.h"
#include "FFTProvider.h"
#include "Math.h"

#include <algorithm>

#pragma once

/// <summary>
/// Implements a wave analyzer to measure relative amplitudes of single frequency components in a complex waveform.
/// </summary>
class SpectrumAnalyzer : public FFTProvider
{
public:
    SpectrumAnalyzer() = delete;

    /// <summary>
    /// Initializes an instance of the class.
    /// </summary>
    /// <param name="channelCount"></param>
    /// <param name="fftSize"></param>
    /// <param name="sampleRate"></param>
    SpectrumAnalyzer(const Configuration * configuration, uint32_t channelCount, uint32_t sampleRate, size_t fftSize) : FFTProvider(channelCount, fftSize)
    {
        if (sampleRate <= 0)
            throw;

        _Configuration = configuration;
        _SampleRate = sampleRate;
        _NyquistFrequency = _SampleRate / 2.;
    }

    SpectrumAnalyzer(const SpectrumAnalyzer &) = delete;
    SpectrumAnalyzer & operator=(const SpectrumAnalyzer &) = delete;
    SpectrumAnalyzer(SpectrumAnalyzer &&) = delete;
    SpectrumAnalyzer & operator=(SpectrumAnalyzer &&) = delete;

    virtual ~SpectrumAnalyzer() { };

    /// <summary>
    /// Gets the band index of the specified frequency.
    /// </summary>
    int GetFFTIndex(double frequency) const
    {
        return (int)(frequency / _NyquistFrequency * ((double) GetFFTSize() / 2.0));
    }

    /// <summary>
    /// Gets the frequency of the band specfied by the index.
    /// </summary>
    int GetFrequency(int index) const
    {
        return (int)(index * _NyquistFrequency / ((double) GetFFTSize() / 2.0));
    }

    /// <summary>
    /// Gets the spectrum from the FFT coefficients.
    /// </summary>
    void GetSpectrum(const std::vector<std::complex<double>> & coefficients, std::vector<FrequencyBand> & freqBands, uint32_t sampleRate, SummationMethod summationMethod)
    {
        for (FrequencyBand & Iter : freqBands)
        {
            const double LoHz = HzToFFTIndex(Min(Iter.Hi, Iter.Lo), coefficients.size(), sampleRate);
            const double HiHz = HzToFFTIndex(Max(Iter.Hi, Iter.Lo), coefficients.size(), sampleRate);

            const int LoIdx = (int) (_Configuration->_SmoothLowerFrequencies ? ::round(LoHz) + 1 : ::ceil(LoHz));
            const int HiIdx = (int) (_Configuration->_SmoothLowerFrequencies ? ::round(HiHz) - 1 : ::floor(HiHz));

            const bool SmoothGainTransition = (_Configuration->_SmoothGainTransition && (summationMethod == SummationMethod::Sum || summationMethod == SummationMethod::RMSSum));
            const bool IsRMS = (summationMethod == SummationMethod::RMS || summationMethod == SummationMethod::RMSSum);

            const double BandGain =  SmoothGainTransition ? ::hypot(1, ::pow(((Iter.Hi - Iter.Lo) * (double) coefficients.size() / (double) sampleRate), (IsRMS ? 0.5 : 1.))) : 1.;

            if (LoIdx <= HiIdx)
            {
                const int OverflowCompensation = Max(HiIdx - LoIdx - (int) coefficients.size(), 0);

                double Value = (summationMethod == SummationMethod::Minimum) ? DBL_MAX : 0.;

                std::vector<double> Values(16);
                int Count = 0;

                for (int Idx = LoIdx; Idx <= HiIdx - OverflowCompensation; ++Idx)
                {
//                  size_t CoefIdx = ((size_t) Idx % coefficients.size() + coefficients.size()) % coefficients.size();
                    size_t CoefIdx = Wrap((size_t) Idx, coefficients.size());

                    double data = std::abs(coefficients[CoefIdx]);

                    switch (summationMethod)
                    {
                        case SummationMethod::Minimum:
                            Value = Min(data, Value);
                            break;

                        case SummationMethod::Maximum:
                            Value = Max(data, Value);
                            break;

                        case SummationMethod::Sum:
                        case SummationMethod::RMS:
                        case SummationMethod::RMSSum:
                        case SummationMethod::Average:
                            Value += ::pow(data, (1.0 + (double) IsRMS));
                            break;

                        case SummationMethod::Median:
                            Values.push_back(data);
                            break;

                        default:
                            Value = data;
                    }

                    ++Count;
                }

                if (((summationMethod == SummationMethod::Average || summationMethod == SummationMethod::RMS) || SmoothGainTransition) && (Count != 0))
                    Value /= Count;
                else
                if (IsRMS)
                    Value = ::sqrt(Value);
                else
                if (summationMethod == SummationMethod::Median)
                    Value = Median(Values);

                Iter.NewValue = Value * BandGain;
            }
            else
            {
                const double Value = Iter.Ctr * (double) coefficients.size() / sampleRate;

                Iter.NewValue = ::fabs(Lanzcos(coefficients, Value, _Configuration->_KernelSize)) * BandGain;
            }
        }
    }

    /// <summary>
    /// Gets the spectrum based on filter bank energies (Mel-Frequency Cepstrum (MFC)).
    /// </summary>
    /// <ref>https://en.wikipedia.org/wiki/Mel-frequency_cepstrum</ref>
    void GetSpectrum(const std::vector<std::complex<double>> & coefficients, std::vector<FrequencyBand> & freqBands, uint32_t sampleRate)
    {
        for (FrequencyBand & Iter : freqBands)
        {
            double Sum = 0;

            const double minBin = Min(Iter.Lo, Iter.Hi) * (double) coefficients.size() / sampleRate;
            const double midBin = Iter.Ctr              * (double) coefficients.size() / sampleRate;
            const double maxBin = Max(Iter.Lo, Iter.Hi) * (double) coefficients.size() / sampleRate;

            const double overflowCompensation = Max(0., maxBin - minBin - (double) coefficients.size());

            for (double i = ::floor(midBin); i >= ::floor(minBin + overflowCompensation); --i)
                Sum += ::pow(std::abs(coefficients[Wrap((size_t) i, coefficients.size())]) * Max(Map(i, minBin, midBin, 0., 1.), 0.), 2.0);

            for (double i = ::ceil(midBin); i <= ::ceil(maxBin - overflowCompensation); ++i)
                Sum += ::pow(std::abs(coefficients[Wrap((size_t) i, coefficients.size())]) * Max(Map(i, maxBin, midBin, 0., 1.), 0.), 2.0);

            Iter.NewValue = ::sqrt(Sum);
        }
    }

    /// <summary>
    /// Applies a Lanzcos kernel to the specified value.
    /// </summary>
    double Lanzcos(const std::vector<complex<double>> & fftCoeffs, double value, int kernelSize) const
    {
        double re = 0.;
        double im = 0.;

        for (int i = -kernelSize + 1; i <= kernelSize; ++i)
        {
            double Pos = ::floor(value) + i;
            double Twiddle = value - Pos; // -Pos + ::round(Pos) + i

            double w = (::fabs(Twiddle) <= 0.) ? 1. : ::sin(Twiddle * M_PI) / (Twiddle * M_PI) * ::sin(M_PI * Twiddle / kernelSize) / (M_PI * Twiddle / kernelSize);

//          size_t CoefIdx = ((size_t) Pos % fftCoeffs.size() + fftCoeffs.size()) % fftCoeffs.size();
            size_t CoefIdx = Wrap((size_t) Pos, fftCoeffs.size());

//          re += fftCoeffs[CoefIdx].real() * w * (-1 + (i % 2 + 2) % 2 * 2);
//          im += fftCoeffs[CoefIdx].imag() * w * (-1 + (i % 2 + 2) % 2 * 2);
            re += fftCoeffs[CoefIdx].real() * w * (-1 + Wrap(i, 2) * 2);
            im += fftCoeffs[CoefIdx].imag() * w * (-1 + Wrap(i, 2) * 2);
        }

        return ::hypot(re, im);
    }

    /// <summary>
    /// Calculates the median.
    /// </summary>
    double Median(std::vector<double> & data)
    {
        if (data.size())
            return NAN;

        if (data.size() <= 1)
            return data[0];

        std::vector<double> SortedData = data;

        std::sort(SortedData.begin(), SortedData.end());

        size_t Half = data.size() / 2;

        return (data.size() % 2) ? SortedData[Half] : (SortedData[Half - 1] + SortedData[Half]) / 2;
    }

    /// <summary>
    /// Calculates the position of the peak indicators.
    /// </summary>
    void UpdatePeakIndicators(std::vector<FrequencyBand> & freqBands)
    {
        for (FrequencyBand & Iter : freqBands)
        {
            double Amplitude = _Configuration->ScaleA(Iter.CurValue);

            if (!::isfinite(Iter.Peak))
                Iter.Peak = 0.;

            if (Amplitude >= Iter.Peak)
            {
                if (_Configuration->_PeakMode == PeakMode::AIMP)
                    Iter.HoldTime = (::isfinite(Iter.HoldTime) ? Iter.HoldTime : 0.) + (Amplitude - Iter.Peak) * _Configuration->_HoldTime;
                else
                    Iter.HoldTime = _Configuration->_HoldTime;

                Iter.Peak = Amplitude;
                Iter.DecaySpeed = 0.;
                Iter.Opacity = 1.;
            }
            else
            if (Iter.HoldTime >= 0.)
            {
                if ((_Configuration->_PeakMode == PeakMode::AIMP))
                    Iter.Peak += (Iter.HoldTime - Max(Iter.HoldTime - 1., 0.)) / _Configuration->_HoldTime;

                Iter.HoldTime -= 1.;

                if (_Configuration->_PeakMode == PeakMode::AIMP)
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
                        break;

                    case PeakMode::Gravity:
                    case PeakMode::FadeOut:
                        Iter.DecaySpeed += _Configuration->_Acceleration / 256.;
                        break;

                    case PeakMode::AIMP:
                        Iter.DecaySpeed = (_Configuration->_Acceleration / 256.) * (1. + (int) (Iter.Peak < 0.5));
                        break;

                        break;
                }

                if (_Configuration->_PeakMode != PeakMode::FadeOut)
                    Iter.Peak -= Iter.DecaySpeed;
                else
                {
                    Iter.Opacity -= Iter.DecaySpeed;

                    if (Iter.Opacity <= 0.)
                        Iter.Peak = 0.;
                }
            }

            Iter.Peak = Clamp(Iter.Peak, 0., 1.);
        }
    }

    /// <summary>
    /// Gets the index of the coefficient corresponding to the specified frequency.
    /// </summary>
    double HzToFFTIndex(double frequency, size_t bufferSize, uint32_t sampleRate)
    {
        return frequency * (double) bufferSize / sampleRate;
    }

    /// <summary>
    /// Gets the frequency corresponding to the specified coefficient index.
    /// </summary>
    double FFTIndexToHz(size_t index, size_t bufferSize, uint32_t sampleRate)
    {
        return (double)((size_t)(index * sampleRate) / bufferSize);
    }

private:
    const Configuration * _Configuration;
    uint32_t _SampleRate;
    double _NyquistFrequency;
};

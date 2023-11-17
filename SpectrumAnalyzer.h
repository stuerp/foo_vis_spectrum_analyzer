
/** $VER: SpectrumAnalyzer.h (2023.11.17) P. Stuer **/

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
    SpectrumAnalyzer(uint32_t channelCount, size_t fftSize, uint32_t sampleRate) : FFTProvider(channelCount, fftSize)
    {
        if (sampleRate <= 0)
            throw;

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
    void GetSpectrum(const std::vector<std::complex<double>> & fftCoeffs, std::vector<FrequencyBand> & freqBands, uint32_t sampleRate, int kernelSize, SummationMethod summationMethod, bool smoothInterp, bool smoothGainTransition)
    {
        for (FrequencyBand & Iter : freqBands)
        {
            const double LoHz = HzToFFTIndex(Min(Iter.Hi, Iter.Lo), fftCoeffs.size(), sampleRate);
            const double HiHz = HzToFFTIndex(Max(Iter.Hi, Iter.Lo), fftCoeffs.size(), sampleRate);

            const int MinIdx1 = (int)                  ::ceil(LoHz);
            const int MaxIdx1 = (int)                 ::floor(HiHz);

            const int MinIdx2 = (int) (smoothInterp ? ::round(LoHz) + 1 : MinIdx1);
            const int MaxIdx2 = (int) (smoothInterp ? ::round(HiHz) - 1 : MaxIdx1);

            double bandGain = smoothGainTransition && (summationMethod == SummationMethod::Sum || summationMethod == SummationMethod::RMSSum) ? ::hypot(1, ::pow(((Iter.Hi - Iter.Lo) * (double) fftCoeffs.size() / sampleRate), (summationMethod == SummationMethod::RMS || summationMethod == SummationMethod::RMSSum) ? 0.5 : 1.)) : 1.;

            if (MinIdx2 > MaxIdx2)
            {
                Iter.NewValue = ::fabs(Lanzcos(fftCoeffs, Iter.Ctr * (double) fftCoeffs.size() / sampleRate, kernelSize)) * bandGain;
            }
            else
            {
                double Value = (summationMethod == SummationMethod::Minimum) ? DBL_MAX : 0.;
                int Count = 0;

                int OverflowCompensation = Max(MaxIdx1 - MinIdx1 - (int) fftCoeffs.size(), 0);

                bool IsAverage = (summationMethod == SummationMethod::Average || summationMethod == SummationMethod::RMS) || ((summationMethod == SummationMethod::Sum || summationMethod == SummationMethod::RMSSum) && smoothGainTransition);
                bool IsRMS = summationMethod == SummationMethod::RMS || summationMethod == SummationMethod::RMSSum;

                std::vector<double> medianData;

                for (int Idx = MinIdx1; Idx <= MaxIdx1 - OverflowCompensation; ++Idx)
                {
                    size_t CoefIdx = ((size_t) Idx % fftCoeffs.size() + fftCoeffs.size()) % fftCoeffs.size();

                    double data = std::abs(fftCoeffs[CoefIdx]);

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
                            medianData.push_back(data);
                            break;

                        default:
                            Value = data;
                    }

                    ++Count;
                }

                if (IsAverage && (Count != 0))
                    Value /= Count;
                else
                if (IsRMS)
                    Value = ::sqrt(Value);
                else
                if (summationMethod == SummationMethod::Median)
                    Value = Median(medianData);

                Iter.NewValue = Value * bandGain;
            }
        }
    }

    /// <summary>
    /// Applies a Lanzcos kernel.
    /// </summary>
    double Lanzcos(const std::vector<complex<double>> & fftCoeffs, double x, int kernelSize) const
    {
        double re = 0.;
        double im = 0.;

        for (int i = -kernelSize + 1; i <= kernelSize; ++i)
        {
            double Pos = ::floor(x) + i;
            double Twiddle = x - Pos; // -Pos + ::round(Pos) + i

            double w = (::fabs(Twiddle) <= 0.) ? 1. : ::sin(Twiddle * M_PI) / (Twiddle * M_PI) * ::sin(M_PI * Twiddle / kernelSize) / (M_PI * Twiddle / kernelSize);

            size_t CoefIdx = ((size_t) Pos % fftCoeffs.size() + fftCoeffs.size()) % fftCoeffs.size();

            re += fftCoeffs[CoefIdx].real() * w * (-1 + (i % 2 + 2) % 2 * 2);
            im += fftCoeffs[CoefIdx].imag() * w * (-1 + (i % 2 + 2) % 2 * 2);
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
    /// Calculates the decay of the peak values.
    /// </summary>
    void GetDecay(std::vector<FrequencyBand> & freqBands)
    {
        for (FrequencyBand & Iter : freqBands)
        {
            double Amplitude = ScaleA(Iter.CurValue);

            if (!::isfinite(Iter.Peak))
                Iter.Peak = 0.;

            if (Amplitude >= Iter.Peak)
            {
                if (_Configuration._PeakMode == PeakMode::AIMP)
                    Iter.HoldTime = (::isfinite(Iter.HoldTime) ? Iter.HoldTime : 0.) + (Amplitude - Iter.Peak) * _Configuration._HoldTime;
                else
                    Iter.HoldTime = _Configuration._HoldTime;

                Iter.Peak = Amplitude;
                Iter.DecaySpeed = 0.;
            }
            else
            if (Iter.HoldTime >= 0.)
            {
                if ((_Configuration._PeakMode == PeakMode::AIMP))
                    Iter.Peak += (Iter.HoldTime - Max(Iter.HoldTime - 1., 0.)) / _Configuration._HoldTime;

                Iter.HoldTime -= 1.;

                if (_Configuration._PeakMode == PeakMode::AIMP)
                    Iter.HoldTime = Min(Iter.HoldTime, _Configuration._HoldTime);
            }
            else
            {
                switch (_Configuration._PeakMode)
                {
                    default:

                    case PeakMode::None:
                        break;

                    case PeakMode::Classic:
                        Iter.DecaySpeed = _Configuration._FallRate / 256.;
                        break;

                    case PeakMode::Gravity:
                        Iter.DecaySpeed += _Configuration._FallRate / 256.;
                        break;

                    case PeakMode::AIMP:
                        Iter.DecaySpeed = (_Configuration._FallRate / 256.) * (1. + (int) (Iter.Peak < 0.5));
                        break;

                    case PeakMode::FadeOut:
                        break;
                }

                Iter.Peak -= Iter.DecaySpeed;
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
    uint32_t _SampleRate;
    double _NyquistFrequency;
};

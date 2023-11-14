
/** $VER: SpectrumAnalyzer.h (2023.11.11) P. Stuer **/

#include "framework.h"

#include "FrequencyBand.h"
#include "FFTProvider.h"
#include "Math.h"

#include <algorithm>

#pragma once

/// <summary>
/// Implements a wave analyzer to measure relative amplitudes of single frequency components in a complex waveform.
/// </summary>
template<class T>
class SpectrumAnalyzer : public FFTProvider<T>
{
public:
    SpectrumAnalyzer() = delete;

    /// <summary>
    /// Initializes an instance of the class.
    /// </summary>
    /// <param name="channelCount"></param>
    /// <param name="fftSize"></param>
    /// <param name="sampleRate"></param>
    SpectrumAnalyzer(uint32_t channelCount, FFTSize fftSize, uint32_t sampleRate) : FFTProvider<T>(channelCount, fftSize)
    {
        if (sampleRate <= 0)
            throw;

        _SampleRate = sampleRate;
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
        int fftSize = (int) GetFFTSize<T>();

        double NyquistFrequency = _SampleRate / 2.0;

        return (int)(frequency / NyquistFrequency * ((double) fftSize / 2.0));
    }

    /// <summary>
    /// Gets the frequency of the band specfied by the index.
    /// </summary>
    int GetFrequency(int index) const
    {
        int fftSize = (int) GetFFTSize<T>();

        double NyquistFrequency = _SampleRate / 2.0;

        return (int)(index * NyquistFrequency / ((double) fftSize / 2.0));
    }

    // Calculates bandpower from FFT (foobar2000 flavored, can be enhanced by using complex FFT coefficients instead of magnitude-only FFT data)
    void calcSpectrum(const std::vector<double> & fftCoeffs, std::vector<FrequencyBand> & freqBands, int interpSize, SummationMethod summationMethod, bool useComplex, bool smoothInterp, bool smoothGainTransition, uint32_t sampleRate, std::vector<double> & spectrum)
    {
        size_t j = 0;

        for (const FrequencyBand & Iter : freqBands)
        {
            const double LoHz = HzToFFTIndex(Min(Iter.Hi, Iter.Lo), fftCoeffs.size(), sampleRate);
            const double HiHz = HzToFFTIndex(Max(Iter.Hi, Iter.Lo), fftCoeffs.size(), sampleRate);

            const int minIdx1 = (int)                  ::ceil(LoHz);
            const int maxIdx1 = (int)                 ::floor(HiHz);

            const int minIdx2 = (int) (smoothInterp ? ::round(LoHz) + 1 : minIdx1);
            const int maxIdx2 = (int) (smoothInterp ? ::round(HiHz) - 1 : maxIdx1);

            double bandGain = smoothGainTransition && (summationMethod == Sum || summationMethod == RMSSum) ? ::hypot(1, ::pow(((Iter.Hi - Iter.Lo) * (double) fftCoeffs.size() / sampleRate), (1 - (int) (summationMethod == RMS || summationMethod == RMSSum) / 2))) : 1.;

            if (minIdx2 > maxIdx2)
            {
                spectrum[j] = ::fabs(Lanzcos(fftCoeffs, Iter.Ctr * (double) fftCoeffs.size() / sampleRate, interpSize, useComplex)) * bandGain;
            }
            else
            {
                double Value = (summationMethod == Minimum) ? DBL_MAX : 0.;
                int Count = 0;

                int overflowCompensation = Max(maxIdx1 - minIdx1 - (int) fftCoeffs.size(), 0);

                bool IsAverage = (summationMethod == Average || summationMethod == RMS) || ((summationMethod == SummationMethod::Sum || summationMethod == RMSSum) && smoothGainTransition);
                bool IsRMS = summationMethod == RMS || summationMethod == RMSSum;

                std::vector<double> medianData;

                for (int i = minIdx1; i <= maxIdx1 - overflowCompensation; ++i)
                {
                    size_t CoefIdx = ((size_t) i % fftCoeffs.size() + fftCoeffs.size()) % fftCoeffs.size();

                    double data = fftCoeffs[CoefIdx];

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
                            Value += ::pow(data, (1.0 + (IsRMS ? 1.0 : 0.0)));
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
                if (summationMethod == Median)
                    Value = median(medianData);

                spectrum[j] = Value * bandGain;
            }

            ++j;
        }
    }

    double Lanzcos(const std::vector<double> & fftCoeffs, double x, int kernelSize = 4, bool useComplex = false)
    {
        double Sum = 0.;

        for (int i = -kernelSize + 1; i <= kernelSize; ++i)
        {
            double pos = ::floor(x) + i;
            double twiddle = x - pos; // -pos + ::round(pos) + i

            double w = ::fabs(twiddle) <= 0 ? 1 : ::sin(twiddle * M_PI) / (twiddle * M_PI) * ::sin(M_PI * twiddle / kernelSize) / (M_PI * twiddle / kernelSize);

            size_t CoefIdx = ((size_t) pos % fftCoeffs.size() + fftCoeffs.size()) % fftCoeffs.size();

            Sum += fftCoeffs[CoefIdx] * w;
        }

        return Sum;
    }

    double Lanzcos(Complex * data, size_t length, double x, int kernelSize = 4, bool useComplex = false)
    {
        Complex Sum = { 0., 0. };

        for (int i = -kernelSize + 1; i <= kernelSize; ++i)
        {
            double pos = ::floor(x) + i; // i + x
            double twiddle = x - pos; // -pos + ::round(pos) + i

            double w = ::fabs(twiddle) <= 0 ? 1 : ::sin(twiddle * M_PI) / (twiddle * M_PI) * ::sin(M_PI * twiddle / kernelSize) / (M_PI * twiddle / kernelSize);
            int idx = (int) (((int) pos % length + length) % length);

            Sum.re += data[idx].re * w * (-1 + (i % 2 + 2) % 2 * 2);
            Sum.im += data[idx].im * w * (-1 + (i % 2 + 2) % 2 * 2);
        }

        return ::hypot(Sum.re, Sum.im);
    }

    double median(std::vector<double> & data)
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

    double HzToFFTIndex(double x, size_t bufferSize, uint32_t sampleRate)
    {
        return x * (double) bufferSize / sampleRate;
    }

    uint32_t FFTIndexToHz(size_t x, size_t bufferSize, uint32_t sampleRate)
    {
        return (uint32_t)((size_t)(x * sampleRate) / bufferSize);
    }

private:
    uint32_t _SampleRate;
};

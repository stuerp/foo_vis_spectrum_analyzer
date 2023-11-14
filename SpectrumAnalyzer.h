
/** $VER: SpectrumAnalyzer.h (2023.11.14) P. Stuer **/

#include "framework.h"

#include "FftProvider.h"

#pragma once

/// <summary>
/// Implements a wave analyzer to measure relative amplitudes of single frequency components in a complex waveform.
/// </summary>
template <class T>
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
    SpectrumAnalyzer(uint32_t channelCount, FFTSize fftSize, uint32_t sampleRate) : FFTProvider(channelCount, fftSize)
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
        int fftSize = (int)GetFFTSize();

        double NyquistFrequency = _SampleRate / 2.0;

        return (int)(frequency / NyquistFrequency * ((double) fftSize / 2.0));
    }

    /// <summary>
    /// Gets the frequency of the band specfied by the index.
    /// </summary>
    int GetFrequency(int index) const
    {
        int fftSize = (int)GetFFTSize();

        double NyquistFrequency = _SampleRate / 2.0;

        return (int)(index * NyquistFrequency / ((double) fftSize / 2.0));
    }

private:
    uint32_t _SampleRate;
};

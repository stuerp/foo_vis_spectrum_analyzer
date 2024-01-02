
/** $VER: SpectrumAnalyzer.h (2024.01.01) P. Stuer **/

#include "framework.h"

#include "Configuration.h"
#include "FFTProvider.h"
#include "FrequencyBand.h"

#include <algorithm>

#pragma once

/// <summary>
/// Implements a wave analyzer to measure relative amplitudes of single frequency components in a complex waveform.
/// </summary>
class SpectrumAnalyzer : public FFTProvider
{
public:
    SpectrumAnalyzer() = delete;

    SpectrumAnalyzer(const SpectrumAnalyzer &) = delete;
    SpectrumAnalyzer & operator=(const SpectrumAnalyzer &) = delete;
    SpectrumAnalyzer(SpectrumAnalyzer &&) = delete;
    SpectrumAnalyzer & operator=(SpectrumAnalyzer &&) = delete;

    virtual ~SpectrumAnalyzer() { };

    /// <summary>
    /// Initializes an instance of the class.
    /// </summary>
    SpectrumAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, size_t fftSize, const Configuration * configuration) : FFTProvider(channelCount, channelSetup, sampleRate, windowFunction, fftSize)
    {
        _Configuration = configuration;
    }

    void GetSpectrum(const std::vector<std::complex<double>> & coefficients, std::vector<FrequencyBand> & freqBands, uint32_t sampleRate, SummationMethod summationMethod) const noexcept;
    void GetSpectrum(const std::vector<std::complex<double>> & coefficients, std::vector<FrequencyBand> & freqBands, uint32_t sampleRate) const noexcept;

    void UpdatePeakIndicators(std::vector<FrequencyBand> & frequencyBands) const noexcept;

private:
    double Lanzcos(const std::vector<complex<double>> & fftCoeffs, double value, int kernelSize) const noexcept;
    double Median(std::vector<double> & data) const noexcept;

    /// <summary>
    /// Gets the band index of the specified frequency.
    /// </summary>
    size_t GetFFTIndex(double frequency) const noexcept
    {
        return (size_t)(frequency / _NyquistFrequency * ((double) GetFFTSize() / 2.));
    }

    /// <summary>
    /// Gets the frequency of the band specfied by the index.
    /// </summary>
    double GetFrequency(int index) const noexcept
    {
        return ((double) index * _NyquistFrequency) / ((double) GetFFTSize() / 2.);
    }

    /// <summary>
    /// Gets the index of the coefficient corresponding to the specified frequency.
    /// </summary>
    double HzToFFTIndex(double frequency, size_t bufferSize, uint32_t sampleRate) const noexcept
    {
        return frequency * (double) bufferSize / sampleRate;
    }

    /// <summary>
    /// Gets the frequency corresponding to the specified coefficient index.
    /// </summary>
    double FFTIndexToHz(size_t index, size_t bufferSize, uint32_t sampleRate) const noexcept
    {
        return (double)((size_t)(index * sampleRate) / bufferSize);
    }

private:
    const Configuration * _Configuration;
};

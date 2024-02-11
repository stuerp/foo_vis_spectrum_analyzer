
/** $VER: FFTAnalyzer.h (2024.01.26) P. Stuer **/

#pragma once

#include "framework.h"

#include "Configuration.h"
#include "FFTProvider.h"
#include "FrequencyBand.h"

/// <summary>
/// Implements a Fast Fourier Transform analyzer.
/// </summary>
class FFTAnalyzer : public FFTProvider
{
public:
    FFTAnalyzer() = delete;

    FFTAnalyzer(const FFTAnalyzer &) = delete;
    FFTAnalyzer & operator=(const FFTAnalyzer &) = delete;
    FFTAnalyzer(FFTAnalyzer &&) = delete;
    FFTAnalyzer & operator=(FFTAnalyzer &&) = delete;

    virtual ~FFTAnalyzer() { };

    FFTAnalyzer(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, size_t fftSize, const Configuration * configuration);

    void AnalyzeSamples(const std::vector<std::complex<double>> & coefficients, uint32_t sampleRate, SummationMethod summationMethod, std::vector<FrequencyBand> & freqBands) const noexcept;
    void AnalyzeSamples(const std::vector<std::complex<double>> & coefficients, uint32_t sampleRate, std::vector<FrequencyBand> & freqBands) const noexcept;
    void AnalyzeSamples(const std::vector<std::complex<double>> & coefficients, uint32_t sampleRate, const WindowFunction & windowFunction, double bandwidthOffset, double bandwidthCap, double bandwidthAmount, bool granularBW, std::vector<FrequencyBand> & freqBands) const noexcept;

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

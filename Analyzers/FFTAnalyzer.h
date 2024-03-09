
/** $VER: FFTAnalyzer.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Analyzer.h"
#include "FrequencyBand.h"

#include "FFT.h"

/// <summary>
/// Implements a Fast Fourier Transform analyzer.
/// </summary>
#pragma warning(disable: 4820)
class FFTAnalyzer : public Analyzer
{
public:
    FFTAnalyzer() = delete;

    FFTAnalyzer(const FFTAnalyzer &) = delete;
    FFTAnalyzer & operator=(const FFTAnalyzer &) = delete;
    FFTAnalyzer(FFTAnalyzer &&) = delete;
    FFTAnalyzer & operator=(FFTAnalyzer &&) = delete;

    virtual ~FFTAnalyzer();

    FFTAnalyzer(const State * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const WindowFunction & windowFunction, const WindowFunction & brownPucketteKernel, size_t fftSize);
    bool AnalyzeSamples(const audio_sample * samples, size_t sampleCount, uint32_t channels, FrequencyBands & frequencyBands) noexcept;

private:
    void Add(const audio_sample * samples, size_t count, uint32_t channels) noexcept;
    void Transform() noexcept;

    void AnalyzeSamples(uint32_t sampleRate, FrequencyBands & freqBands) const noexcept;
    void AnalyzeSamplesUsingTFB(uint32_t sampleRate, FrequencyBands & freqBands) const noexcept;
    void AnalyzeSamplesUsingBP(uint32_t sampleRate, FrequencyBands & freqBands) const noexcept;

    double Lanzcos(const std::vector<complex<double>> & fftCoeffs, double value, int kernelSize) const noexcept;
    double Median(std::vector<double> & data) const noexcept;

    /// <summary>
    /// Gets the current FFT size.
    /// </summary>
    size_t GetFFTSize() const
    {
        return _FFTSize;
    }

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
    FFT _FFT;
    size_t _FFTSize;

    // Wrap-around sample buffer
    audio_sample * _Data;
    size_t _Size;
    size_t _Curr;

    vector<complex<double>> _TimeData;
    vector<complex<double>> _FreqData;

    const WindowFunction & _BrownPucketteKernel;
};


/** $VER: FFTProvider.h (2024.01.02) P. Stuer **/

#pragma once

#include "framework.h"

#include "TransformProvider.h"
#include "FFT.h"

using namespace std;

/// <summary>
/// Implements a Fast Fourier Transform provider.
/// </summary>
class FFTProvider : public TransformProvider
{
public:
    FFTProvider() = delete;

    FFTProvider(const FFTProvider &) = delete;
    FFTProvider & operator=(const FFTProvider &) = delete;
    FFTProvider(FFTProvider &&) = delete;
    FFTProvider & operator=(FFTProvider &&) = delete;

    virtual ~FFTProvider();

    FFTProvider(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, size_t fftSize);

    void Add(const audio_sample * samples, size_t count, uint32_t channelMask) noexcept;
    void GetFrequencyCoefficients(vector<complex<double>> & freqData) noexcept;

    size_t GetFFTSize() const
    {
        return _FFTSize;
    }

private:
    FFT _FFT;
    size_t _FFTSize;

    audio_sample * _Data;
    size_t _Size;
    size_t _Curr;

    vector<complex<double>> _TimeData;
};

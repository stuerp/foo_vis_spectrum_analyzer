
/** $VER: FFTProvider.h (2022.11.18) P. Stuer **/

#pragma once

#define kiss_fft_scalar double

#include "FFT.h"

/// <summary>
/// Implements a Fast Fourier Transform provider.
/// </summary>
class FFTProvider
{
public:
    FFTProvider(t_size channelCount, FFTSize fftSize);
    ~FFTProvider();

    t_size GetChannelCount() const
    {
        return _ChannelCount;
    }

    FFTSize GetFFTSize() const
    {
        return _FFTSize;
    }

    bool Add(const audio_sample * samples, size_t count) noexcept;
    bool GetFrequencyData(double * freqData, size_t freqSize) noexcept;

private:
    FFTProvider();
    FFTProvider(const FFTProvider&);

    bool GetFrequencyData(kiss_fft_cpx * freqData) noexcept;

    static audio_sample AverageSamples(const audio_sample * samples, size_t i, size_t channelCount);

private:
    size_t _ChannelCount;
    FFTSize _FFTSize;

    audio_sample * _SampleData;
    size_t _SampleSize;
    size_t _SampleCount;

    FFT _FFT;
};

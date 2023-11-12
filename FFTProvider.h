
/** $VER: FFTProvider.h (2023.11.12) P. Stuer **/

#pragma once

#include "FFT.h"

/// <summary>
/// Implements a Fast Fourier Transform provider.
/// </summary>
class FFTProvider
{
public:
    FFTProvider(t_size channelCount, FFTSize fftSize);

    virtual ~FFTProvider();

    void Reset()
    {
        _SampleCount = 0;
    }

    size_t GetChannelCount() const
    {
        return _ChannelCount;
    }

    FFTSize GetFFTSize() const
    {
        return _FFTSize;
    }

    bool Add(const audio_sample * samples, size_t count) noexcept;
    bool GetFrequencyData(double * freqData, size_t freqSize) const noexcept;

private:
    FFTProvider();
    FFTProvider(const FFTProvider&);

    bool GetFrequencyData(kiss_fft_cpx * freqData) const noexcept;

    static audio_sample AverageSamples(const audio_sample * samples, size_t i, size_t channelCount);

private:
    size_t _ChannelCount;
    FFTSize _FFTSize;

    audio_sample * _SampleData;
    size_t _SampleSize;
    size_t _SampleCount;

    FFT _FFT;
};


/** $VER: FFTProvider.h (2023.11.14) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "FFT.h"

/// <summary>
/// Implements a Fast Fourier Transform provider.
/// </summary>
template <class T>
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

    bool Add(const T * samples, size_t count) noexcept;
    bool GetFrequencyData(double * freqData, size_t freqSize) const noexcept;

private:
    FFTProvider();
    FFTProvider(const FFTProvider&);

    bool GetFrequencyData(kiss_fft_cpx * freqData) const noexcept;

    static T AverageSamples(const T * samples, size_t i, size_t channelCount);

private:
    size_t _ChannelCount;
    FFTSize _FFTSize;

    T * _SampleData;
    size_t _SampleSize;
    size_t _SampleCount;

    FFT _FFT;
};

#define IsPowerOfTwo(x) ((x) & ((x) - 1) == 0)

/// <summary>
/// Initializes a new instance of the class.
/// </summary>
/// <param name="channelCount">Number of channels of the input data</param>
/// <param name="fftSize">The number of bands to use</param>
FFTProvider<class T>::FFTProvider(t_size channelCount, FFTSize fftSize)
{
    if (channelCount == 0)
        throw; // FIXME

    // fftSize must be a power of 2 in the range 32 to 32768.
//  if ((fftSize < 32) || (fftSize > 32768) || !IsPowerOfTwo(fftSize))
//      throw; // FIXME

    _ChannelCount = channelCount;
    _FFTSize = fftSize;

    _FFT.Initialize(_FFTSize);

    // Create the ring buffer for the samples. The size should be a multiple of 2.
    _SampleData = new T[(size_t) fftSize];
    _SampleSize = (size_t) fftSize;

    ::memset(_SampleData, 0, sizeof(T) * _SampleSize);

    _SampleCount = 0;
}

/// <summary>
/// 
/// </summary>
FFTProvider<class T>::~FFTProvider()
{
    if (_SampleData)
    {
        delete[] _SampleData;
        _SampleData = nullptr;
    }
}

/// <summary>
/// Adds multiple samples to the provider.
/// It assumes that the buffer contains tuples of sample data for each channel. E.g. for 2 channels: Left(0), Right(0), Left(1), Right(1) ... Left(n), Right(n)
/// </summary>
/// <param name="samples">Double array that contains samples</param>
/// <param name="sampleCount">Number of samples to add to the provider</param>
bool FFTProvider<class T>::Add(const T * samples, size_t sampleCount) noexcept
{
    if (samples == nullptr)
        return false;

    // Make sure there are enough samples for all the channels.
    sampleCount -= (sampleCount % _ChannelCount);

    // Merge the samples of all channels into one averaged sample.
    for (size_t i = 0; i < sampleCount; i += _ChannelCount)
    {
        _SampleData[_SampleCount++] = AverageSamples(samples, i, _ChannelCount);

        // Wrap around the buffer.
        if (_SampleCount >= _SampleSize)
            _SampleCount = 0;
    }

    return true;
}

/// <summary>
/// Calculates the Fast Fourier Transform and stores the result in fftData.
/// </summary>
/// <param name="freqData">The output buffer containing the magnitude of the frequencies.</param>
/// <param name="freqSize"></param>
/// <returns>
/// Returns a value which indicates whether the Fast Fourier Transform got calculated.
/// If there have not been added any new samples since the last transform, the FFT
/// won't be calculated. True means that the Fast Fourier Transform got calculated.
/// </returns>
bool FFTProvider<class T>::GetFrequencyData(double * freqData, size_t freqSize) const noexcept
{
    if ((freqData == nullptr) || (freqSize != (size_t) _FFTSize))
        return false;

    {
        // FIXME: Don't new/delete with every transform!
        // Create the buffer for the frequency domain data.
        kiss_fft_cpx * FreqData = new kiss_fft_cpx[(size_t) _FFTSize / 2 + 1];

        if (FreqData == nullptr)
            return false;

        GetFrequencyData(FreqData);

        // Compute the magnitude of each of the frequencies.
        for (size_t i = 0; i < (size_t) _FFTSize / 2; i++)
        {
            double Magnitude = ::sqrt((FreqData[i].r * FreqData[i].r) + (FreqData[i].i * FreqData[i].i));

            freqData[i] =  Magnitude;
        }

        delete[] FreqData;
    }

    return true;
}

/// <summary>
/// Calculates the Fast Fourier Transform and returns the frequency data in the result buffer.
/// </summary>
/// <param name="result">The output buffer</param>
/// <returns>
/// Returns a value which indicates whether the Fast Fourier Transform got calculated.
/// If there have not been added any new samples since the last transform, the FFT
/// won't be calculated. True means that the Fast Fourier Transform got calculated.
///</returns>
bool FFTProvider<class T>::GetFrequencyData(kiss_fft_cpx * freqData) const noexcept
{
    //FIXME Don't reallocate this buffer all the time.
    T * TimeData = new T[_FFTSize];

    if (TimeData == nullptr)
        return false;

//  ::memset(TimeData, 0, sizeof(T) * _FFTSize);

    {
        T * t = TimeData;

        // Fill the FFT buffer from the wrap-around sample buffer with Time domain data.
        size_t SamplesToCopy = _SampleSize - _SampleCount;

        ::memcpy(t, &_SampleData[_SampleCount], SamplesToCopy * sizeof(T));

        t += SamplesToCopy;

        SamplesToCopy = _SampleCount;

        ::memcpy(t, &_SampleData[0], SamplesToCopy * sizeof(T));
    }

    // Apply the windowing function.
    T Norm = 0.0;

    for (int i = 0; i < _FFTSize; ++i)
    {
        T Multiplier = (T) FFT::HanningWindow(i, _FFTSize);

        TimeData[i] *= Multiplier;

        Norm += Multiplier;
    }
/*
    // Normalize the time domain data.
    double Factor = (double) _FFTSize / Norm / M_SQRT2;

    for (int i = 0; i < _FFTSize; ++i)
        TimeData[i] *= Factor;
*/
    // Transform the data from the Time domain to the Frequency domain.
    _FFT.Transform((const kiss_fft_scalar *) TimeData, freqData);

    // Normalize the frequency domain data. Use the size of the FFT for dB scale. FIXME: Determine scale factor for a logaritmic scale.
    for (size_t i = 0; i < (size_t) _FFTSize / 2; ++i)
    {
        freqData[i].r /= (kiss_fft_scalar) _FFTSize;
        freqData[i].i /= (kiss_fft_scalar) _FFTSize;
    }

    delete[] TimeData;

    return true;
}

/// <summary>
/// Calculates the average of the specified samples.
/// </summary>
T FFTProvider<class T>::AverageSamples(const T * samples, size_t i, size_t channelCount)
{
    switch (channelCount)
    {
        case 1:
            return samples[i];

        case 2:
            return (samples[i] + samples[i + 1]) / (T) 2.0;

        case 3:
            return (samples[i] + samples[i + 1] + samples[i + 2]) / (T) 3.0;

        case 4:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3]) / (T) 4.0;

        case 5:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4]) / (T) 5.0;

        case 6:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4] + samples[i + 5]) / (T) 6.0;

        default:
        {
            T Average = 0.;

            for (size_t j = 0; j < channelCount; j++)
                Average += samples[i + j];

            return Average / (T) channelCount;
        }
    }
}

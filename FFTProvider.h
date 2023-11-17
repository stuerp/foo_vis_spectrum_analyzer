
/** $VER: FFTProvider.h (2023.11.16) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "FFT.h"

using namespace std;

/// <summary>
/// Implements a Fast Fourier Transform provider.
/// </summary>
class FFTProvider
{
public:
    FFTProvider() = delete;
    FFTProvider(size_t channelCount, size_t fftSize);

    FFTProvider(const FFTProvider &) = delete;
    FFTProvider & operator=(const FFTProvider &) = delete;
    FFTProvider(FFTProvider &&) = delete;
    FFTProvider & operator=(FFTProvider &&) = delete;

    virtual ~FFTProvider();

    size_t GetChannelCount() const
    {
        return _ChannelCount;
    }

    size_t GetFFTSize() const
    {
        return _FFTSize;
    }

    bool Add(const audio_sample * samples, size_t count) noexcept;
    bool GetFrequencyData(vector<complex<double>> & freqData) noexcept;

private:
    static audio_sample AverageSamples(const audio_sample * samples, size_t i, size_t channelCount);

private:
    size_t _ChannelCount;
    size_t _FFTSize;

    FFT _FFT;

    audio_sample * _SampleData;
    size_t _SampleSize;
    size_t _SampleCount;

    vector<complex<double>> _TimeData;
};

/// <summary>
/// Initializes a new instance of the class.
/// </summary>
/// <param name="channelCount">Number of channels of the input data</param>
/// <param name="fftSize">The number of bands to use</param>
inline FFTProvider::FFTProvider(size_t channelCount, size_t fftSize)
{
    if (channelCount == 0)
        throw; // FIXME

    _ChannelCount = channelCount;
    _FFTSize = fftSize;

    _FFT.Initialize(fftSize);
    _TimeData.resize((size_t) fftSize);

    // Create the ring buffer for the samples. The size should be a multiple of 2.
    _SampleData = new audio_sample[(size_t) fftSize];
    _SampleSize = (size_t) fftSize;

    ::memset(_SampleData, 0, sizeof(audio_sample) * _SampleSize);

    _SampleCount = 0;
}

/// <summary>
/// Destroys this instance.
/// </summary>
inline FFTProvider::~FFTProvider()
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
inline bool FFTProvider::Add(const audio_sample * samples, size_t sampleCount) noexcept
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
/// Calculates the Fast Fourier Transform and returns the frequency data in the result buffer.
/// </summary>
/// <param name="result">The output buffer</param>
/// <returns>
/// Returns a value which indicates whether the Fast Fourier Transform got calculated.
/// If there have not been added any new samples since the last transform, the FFT
/// won't be calculated. True means that the Fast Fourier Transform got calculated.
///</returns>
bool inline FFTProvider::GetFrequencyData(vector<complex<double>> & freqData) noexcept
{
    double Norm = 0.0;

    // Fill the FFT buffer from the wrap-around sample buffer with Time domain data and apply the windowing function.
    {
        size_t i = _SampleCount;
        size_t j = 0;

        for (complex<double> & Iter : _TimeData)
        {
            double Multiplier = FFT::HanningWindow(j, _FFTSize);

            Iter.real(_SampleData[i] * Multiplier);

            if (++i == _SampleSize)
                i = 0;

            Norm += Multiplier;
            j++;
        }
    }

    // Normalize the Time domain data.
    {
        double Factor = (double) _FFTSize / Norm / M_SQRT2;

        for (complex<double> & Iter : _TimeData)
            Iter.real(Iter.real() * Factor);
    }

    // Transform the data from the Time domain to the Frequency domain.
    _FFT.Transform(_TimeData, freqData);

    // Normalize the frequency domain data.
    for (complex<double> & Iter : freqData)
        Iter /= _FFTSize;

    return true;
}

/// <summary>
/// Calculates the average of the specified samples.
/// </summary>
audio_sample inline FFTProvider::AverageSamples(const audio_sample * samples, size_t i, size_t channelCount)
{
    switch (channelCount)
    {
        case 1:
            return samples[i];

        case 2:
            return (samples[i] + samples[i + 1]) / (audio_sample) 2.0;

        case 3:
            return (samples[i] + samples[i + 1] + samples[i + 2]) / (audio_sample) 3.0;

        case 4:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3]) / (audio_sample) 4.0;

        case 5:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4]) / (audio_sample) 5.0;

        case 6:
            return (samples[i] + samples[i + 1] + samples[i + 2] + samples[i + 3] + samples[i + 4] + samples[i + 5]) / (audio_sample) 6.0;

        default:
        {
            audio_sample Average = 0.;

            for (size_t j = 0; j < channelCount; ++j)
                Average += samples[i + j];

            return Average / (audio_sample) channelCount;
        }
    }
}

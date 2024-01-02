
/** $VER: FFTProvider.cpp (2024.01.02) P. Stuer **/

#include "FFTProvider.h"

#include "Support.h"
#include "Log.h"

#pragma hdrstop

/// <summary>
/// Destroys this instance.
/// </summary>
FFTProvider::~FFTProvider()
{
    if (_Data)
    {
        delete[] _Data;
        _Data = nullptr;
    }
}

/// <summary>
/// Initializes a new instance.
/// </summary>
FFTProvider::FFTProvider(uint32_t channelCount, uint32_t channelSetup, double sampleRate, const WindowFunction & windowFunction, size_t fftSize) : TransformProvider(channelCount, channelSetup, sampleRate, windowFunction)
{
    _FFTSize = fftSize;

    _FFT.Initialize(_FFTSize);
    _TimeData.resize(_FFTSize);

    // Create the ring buffer for the samples.
    _Size = _FFTSize;
    _Data = new audio_sample[_Size];

    ::memset(_Data, 0, sizeof(audio_sample) * _Size);

    _Curr = 0;
}


/// <summary>
/// Adds multiple samples to the provider.
/// It assumes that the buffer contains tuples of sample data for each channel. E.g. for 2 channels: Left(0), Right(0), Left(1), Right(1) ... Left(n), Right(n)
/// </summary>
/// <param name="samples">Array that contains samples</param>
/// <param name="sampleCount">Number of samples to add to the provider</param>
void FFTProvider::FFTProvider::Add(const audio_sample * samples, size_t sampleCount, uint32_t channelMask) noexcept
{
    if (samples == nullptr)
        return;

    // Make sure there are enough samples for all the channels.
    sampleCount -= (sampleCount % _ChannelCount);

    // Merge the samples of all channels into one averaged sample.
    for (size_t i = 0; i < sampleCount; i += _ChannelCount)
    {
        _Data[_Curr] = AverageSamples(&samples[i], channelMask);

        // Wrap around the buffer index.
        if (++_Curr == _Size)
            _Curr = 0;
    }
}

/// <summary>
/// Calculates the Fast Fourier Transform and returns the frequency data in the result buffer.
/// </summary>
void FFTProvider::FFTProvider::GetFrequencyCoefficients(vector<complex<double>> & freqCoefficients) noexcept
{
    double Norm = 0.;

    // Fill the FFT buffer from the sample ring buffer with Time domain data and apply the windowing function.
    {
        size_t i = _Curr;
        size_t j = 0;

        for (complex<double> & Iter : _TimeData)
        {
            double WindowFactor = _WindowFunction(Map(j, (size_t) 0, _FFTSize, -1., 1.));

            Iter = complex<double>(_Data[i] * WindowFactor, 0.);

            if (++i == _Size)
                i = 0;

            Norm += WindowFactor;
            j++;
        }
    }

    // Normalize the Time domain data.
    {
        const double Factor = (double) _FFTSize / Norm / M_SQRT2;

        for (complex<double> & Iter : _TimeData)
            Iter *= Factor;
    }

    // Transform the data from the Time domain to the Frequency domain.
    _FFT.Transform(_TimeData, freqCoefficients);

    // Normalize the Frequency domain data.
    for (complex<double> & Iter : freqCoefficients)
        Iter /= (double) _FFTSize / 2.;
}

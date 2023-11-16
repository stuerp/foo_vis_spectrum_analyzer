
/** $VER: FFTKiss.h (2023.11.16) P. Stuer - A wrapper for kissfft **/

#pragma once

#include <new>

#include <math.h>

#include <kiss_fftr.h>

#include "FFTBase.h"

/// <summary>
/// Implements a wrapper for kissfft.
/// </summary>
class FFT : public FFTBase
{
public:
    /// <summary>
    /// Initializes a new instance of the class.
    /// </summary>
    FFT() : _Config(nullptr)
    {
    }

    /// <summary>
    /// Frees all allocated resources.
    /// </summary>
    ~FFT()
    {
        Dispose();
    }

    /// <summary>
    /// Initializes the instance.
    /// </summary>
    /// <param name="fftSize"></param>
    bool Initialize(FFTSize fftSize)
    {
        if (((size_t) fftSize > 0) && (fftSize == (FFTSize) _FFTSize))
            return false;

        Dispose();

        _FFTSize = (int) fftSize;

        _Config = ::kiss_fftr_alloc(_FFTSize, IsInverseFFT, nullptr, nullptr);

        if (_Config == nullptr)
            return false;

        return true;
    }

    const int IsInverseFFT = 0;

    /// <summary>
    /// Computes the Fast Fourier Transform.
    /// </summary>
    /// <param name="timeData"></param>
    /// <param name="freqData">freqData[0] = DC; freqData[1] = 1Hz; freqData[fftSize / 2] = Nyquist frequency</param>
    /// <returns></returns>
    bool Transform(const kiss_fft_scalar * timeData, kiss_fft_cpx * freqData) const
    {
        if (_Config == nullptr)
            return false;

        ::kiss_fftr(_Config, timeData, freqData);

        return true;
    }

private:
    /// <summary>
    /// Disposes all allocated resources.
    /// </summary>
    void Dispose() noexcept
    {
        if (_Config)
        {
            ::kiss_fft_free(_Config);
            _Config = nullptr;
        }
    }

private:
    int _FFTSize;
    kiss_fftr_cfg _Config;
};

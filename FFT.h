
/** $VER: FFT.h (2023.11.11) P. Stuer **/

#pragma once

#include <new>

#define _USE_MATH_DEFINES

#include <math.h>

#include <kiss_fftr.h>

/// <summary>
/// Defines FFT data size constants that can be used for FFT calculations.
/// Note that only the half of the specified size can be used for visualizations.
/// </summary>
enum FFTSize
{
    Fft64    =    64, // bands
    Fft128   =   128,
    Fft256   =   256,
    Fft512   =   512,
    Fft1024  =  1024,
    Fft2048  =  2048,
    Fft4096  =  4096,
    Fft8192  =  8192,
    Fft16384 = 16384,
    Fft32768 = 32768
};

/// <summary>
/// Implements a wrapper for kissfft.
/// </summary>
class FFT
{
public:
    /// <summary>
    /// Initializes a new instance of the class.
    /// </summary>
    FFT() : _FFTSize(0), _Config(nullptr)
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

    const int IsInverseFFT = 0;

    /// <summary>
    /// Implements the Hanning Window function.
    /// </summary>
    /// <param name="n">Current index of the input signal</param>
    /// <param name="N">Window width</param>
    /// <returns>Hanning window multiplier</returns>
    static double HanningWindow(int n, int N)
    {
        return (0.5 * (1.0 - ::cos(M_PI * 2.0 * (double) n / (double) (N - 1))));
    }

    /// <summary>
    /// Implements the Hamming Window function.
    /// </summary>
    /// <param name="n">Current index of the input signal</param>
    /// <param name="N">Window width</param>
    /// <returns>Hamming window multiplier</returns>
    static double HammingWindow(int n, int N)
    {
        return 0.53836 - 0.46164 * ::cos(M_PI * 2.0 * (double) n / (double) (N - 1));
    }

private:
    /// <summary>
    /// Frees all allocated resources.
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

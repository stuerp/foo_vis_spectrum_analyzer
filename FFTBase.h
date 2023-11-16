
/** $VER: FFTBase.h (2023.11.16) P. Stuer **/

#pragma once

#include <vector>
#include <complex>

using namespace std;

#include "FFT.h"

/// <summary>
/// Provides an abstract base class for FFTs.
/// </summary>
class FFTBase
{
public:
    /// <summary>
    /// Initializes a new instance of the class.
    /// </summary>
    FFTBase() { }

    /// <summary>
    /// Frees all allocated resources.
    /// </summary>
    virtual ~FFTBase() { };

    /// <summary>
    /// Initializes the instance.
    /// </summary>
    /// <param name="fftSize"></param>
    /// <returns>True if the initialization succeeded.</returns>
    virtual bool Initialize(size_t fftSize) = 0;

    /// <summary>
    /// Computes the Fast Fourier Transform.
    /// </summary>
    /// <param name="timeData"></param>
    /// <param name="freqData">freqData[0] = DC; freqData[1] = 1Hz; freqData[fftSize / 2] = Nyquist frequency</param>
    /// <returns>True if the transformation succeeded.</returns>
    bool Transform(const vector<complex<double>> &, const vector<complex<double>> &) const
    {
        return true;
    }

    /// <summary>
    /// Implements the Hanning Window function.
    /// </summary>
    /// <param name="n">Current index of the input signal</param>
    /// <param name="N">Window width</param>
    /// <returns>Hanning window multiplier</returns>
    static double HanningWindow(size_t n, size_t N)
    {
        return (0.5 * (1.0 - ::cos(M_PI * 2.0 * (double) n / ((double) N - 1.0))));
    }

    /// <summary>
    /// Implements the Hamming Window function.
    /// </summary>
    /// <param name="n">Current index of the input signal</param>
    /// <param name="N">Window width</param>
    /// <returns>Hamming window multiplier</returns>
    static double HammingWindow(size_t n, size_t N)
    {
        return 0.53836 - 0.46164 * ::cos(M_PI * 2.0 * (double) n / ((double) N - 1.0));
    }
};

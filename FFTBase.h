
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
};

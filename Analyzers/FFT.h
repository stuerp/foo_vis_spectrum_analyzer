
/** $VER: FFT.h (2023.12.29) P. Stuer **/

#pragma once

#include <FftComplex.hpp>

#include <vector>
#include <complex>

using namespace std;

/// <summary>
/// Implements a wrapper for the complex FFT from Project Nayuki (https://www.nayuki.io/page/free-small-fft-in-multiple-languages).
/// </summary>
class FFT
{
public:
    /// <summary>
    /// Initializes a new instance of the class.
    /// </summary>
    FFT() : _FFTSize() { }

    /// <summary>
    /// Frees all allocated resources.
    /// </summary>
    ~FFT() { }

    /// <summary>
    /// Initializes the instance.
    /// </summary>
    /// <param name="fftSize"></param>
    bool Initialize(size_t fftSize)
    {
        _FFTSize = fftSize;

        return true;
    }

    /// <summary>
    /// Computes the Fast Fourier Transform.
    /// </summary>
    /// <param name="timeData"></param>
    /// <param name="freqData">freqData[0] = DC; freqData[1] = 1Hz; freqData[fftSize / 2] = Nyquist frequency</param>
    bool Transform(const vector<complex<double>> & timeData, std::vector<complex<double>> & freqData) noexcept
    {
        freqData = timeData;

        try
        {
            Fft::transform(freqData, _Exp, false);
        }
        catch (exception)
        {
            std::fill(freqData.begin(), freqData.end(), 0.);
        }

        return true;
    }

private:
    size_t _FFTSize;

    vector<complex<double>> _Exp; // Trigonometric table
};

#pragma once

/// <summary>
/// Defines FFT data size constants that can be used for FFT calculations.
/// Note that only the half of the specified size can be used for visualizations.
/// </summary>
enum FFTSize
{
    Fft64    =    64, // Number of frequency bins
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

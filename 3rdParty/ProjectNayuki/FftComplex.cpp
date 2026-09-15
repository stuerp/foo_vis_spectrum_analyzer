
/** $VER: FFTComplex.cpp (2026.09.13) P. Stuer - Modified version of the original Nayuki code **/

/*
 * Free FFT and convolution (C++)
 *
 * Copyright (c) 2021 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/free-small-fft-in-multiple-languages
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#include "pch.h"

#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include "FftComplex.hpp"

using std::complex;
using std::size_t;
using std::uintmax_t;
using std::vector;

// Private function prototypes
[[nodiscard]]
constexpr size_t reverseBits(std::size_t value, unsigned width) noexcept
{
    size_t result = 0;

    while (width--)
    {
        result = (result << 1) | (value & (size_t) 1);

        value >>= 1;
    }

    return result;
}

void Fft::transform(vector<complex<double>> & vec, bool inverse, trig_t & trig)
{
    const size_t n = vec.size();

    if (n == 0)
        return;

    if (std::has_single_bit(n)) // Is power of 2?
        transformRadix2(vec, inverse, trig);
    else  // More complicated algorithm for arbitrary sizes
        transformBluestein(vec, inverse, trig);
}

void Fft::transformRadix2(vector<complex<double>> & vec, bool inverse, trig_t & trig)
{
    // Length variables
    const size_t n = vec.size();

    if (!std::has_single_bit(n))
        throw std::domain_error("Length is not a power of 2");

    const auto Levels = (unsigned int) (std::bit_width(n) - 1);

    if ((trig._Exp.size() != n / 2) || (trig._Inverse != inverse))
    {
        trig._Exp.resize(n / 2);

        for (size_t i = 0; i < n / 2; ++i)
        {
            const double angle = (inverse ? 2 : -2) * std::numbers::pi * (double) i / (double) n;

            trig._Exp[i] = std::polar(1.0, angle);
        }

        trig._Inverse = inverse;
    }

    // Bit-reversed addressing permutation
    for (size_t i = 0; i < n; ++i)
    {
        const size_t j = reverseBits(i, Levels);

        if (j > i)
            std::swap(vec[i], vec[j]);
    }

    // Cooley-Tukey decimation-in-time radix-2 FFT
    for (size_t size = 2; size <= n; size *= 2)
    {
        const size_t halfsize = size / 2;
        const size_t tablestep = n / size;

        for (size_t i = 0; i < n; i += size)
        {
            for (size_t j = i, k = 0; j < i + halfsize; ++j, k += tablestep)
            {
                const complex<double> temp = vec[j + halfsize] * trig._Exp[k];

                vec[j + halfsize] = vec[j] - temp;
                vec[j] += temp;
            }
        }

        if (size == n)  // Prevent overflow in 'size *= 2'
            break;
    }
}

void Fft::transformBluestein(vector<complex<double>> & vec, bool inverse, trig_t & trig)
{
    // Find a power-of-2 convolution length m such that m >= n * 2 + 1
    const size_t n = vec.size();

    size_t m = 1;

    while (m / 2 <= n)
    {
        if (m > std::numeric_limits<size_t>::max() / 2)
            throw std::length_error("Vector too large");

        m *= 2;
    }

    vector<complex<double>> expTable(n);

    for (size_t i = 0; i < n; ++i)
    {
        const uintmax_t temp = ((uintmax_t) i * i) % ((uintmax_t) n * 2);

        const double angle = (inverse ? std::numbers::pi : -std::numbers::pi) * (double) temp / (double) n;

        expTable[i] = std::polar(1.0, angle);
    }

    // Temporary vectors and preprocessing
    vector<complex<double>> avec(m);

    for (size_t i = 0; i < n; ++i)
        avec[i] = vec[i] * expTable[i];

    vector<complex<double>> bvec(m);

    bvec[0] = expTable[0];

    for (size_t i = 1; i < n; ++i)
        bvec[i] = bvec[m - i] = std::conj(expTable[i]);

    // Convolution
    vector<complex<double>> cvec = convolve(std::move(avec), std::move(bvec), trig);

    // Postprocessing
    for (size_t i = 0; i < n; ++i)
        vec[i] = cvec[i] * expTable[i];
}

vector<complex<double>> Fft::convolve(vector<complex<double>> xvec, vector<complex<double>> yvec, trig_t & trig)
{
    const size_t n = xvec.size();

    if (n != yvec.size())
        throw std::domain_error("Mismatched lengths");

    transform(xvec, false, trig);
    transform(yvec, false, trig);

    for (size_t i = 0; i < n; ++i)
        xvec[i] *= yvec[i];

    transform(xvec, true, trig);

    for (size_t i = 0; i < n; ++i)  // Scaling (because this FFT implementation omits it)
        xvec[i] /= (double) n;

    return xvec;
}

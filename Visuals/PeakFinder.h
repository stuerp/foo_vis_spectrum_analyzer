/*
    The functions presented in this file were originally developed by Clayder Gonzalez,
    Erwin Meza Vega and Hakim Benoudjit, the original code is available here: https://github.com/claydergc/find-peaks
    and the license associated with this code is available at the root of this project under the name LICENSE_FINDPEAKS.

    The same code is taken from the Matlab File Exchange and was written by Nathanael Yoder.

    Some functions were probably adapted to my specific needs for this program and if you wish to reuse my implementation
    please credit the original authors.
*/

#ifndef PEAKFINDER_H
#define PEAKFINDER_H

#include <vector>

namespace PeakFinder {
    const float EPS = 2.2204e-16f;

    /*
        Inputs
        x0: input signal
        extrema: 1 if maxima are desired, -1 if minima are desired
        includeEndpoints - If true the endpoints will be included as possible extrema otherwise they will not be included
        Output
        peakInds: Indices of peaks in x0
    */
    void findPeaks(std::vector<float> x0, std::vector<int>& peakInds, bool includeEndpoints=true, float extrema=1);
}

#endif
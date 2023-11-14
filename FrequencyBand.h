
/** $VER: FrequencyBand.h (2023.11.14) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

struct FrequencyBand
{
    FrequencyBand() : lo(), ctr(), hi() { }
    FrequencyBand(double l, double c, double h) : lo(l), ctr(c), hi(h) { }

    double lo;
    double ctr;
    double hi;
};

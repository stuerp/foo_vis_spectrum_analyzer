
/** $VER: FrequencyBand.h (2023.11.14) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

struct FrequencyBand
{
    FrequencyBand() : Lo(), Ctr(), Hi(), LoBound(), HiBound() { }
    FrequencyBand(double l, double c, double h) : Lo(l), Ctr(c), Hi(h), LoBound(), HiBound() { }

    double Lo;
    double Ctr;
    double Hi;

    double LoBound;
    double HiBound;
};

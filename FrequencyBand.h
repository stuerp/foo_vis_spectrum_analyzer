
/** $VER: FrequencyBand.h (2023.11.18) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

struct FrequencyBand
{
    FrequencyBand() : Lo(), Ctr(), Hi(), NewValue(), CurValue() { }
    FrequencyBand(double l, double c, double h) : Lo(l), Ctr(c), Hi(h), NewValue(), CurValue() { }

    double Lo;
    double Ctr;
    double Hi;

//  double LoBound;
//  double HiBound;

    double NewValue;
    double CurValue;

    double Peak;        // The value of the indicator (0.0 - 1.0)
    double HoldTime;    // Time to hold the current peak value.
    double DecaySpeed;  // Speed at which the current peak value decays.
    double Opacity;     // The opacity of the indicator (0.0 - 1.0)
};

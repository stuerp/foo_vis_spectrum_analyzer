
/** $VER: FrequencyBand.h (2023.12.30) P. Stuer **/

#pragma once

#include "framework.h"

struct FrequencyBand
{
    FrequencyBand() : Lo(), Ctr(), Hi(), NewValue(), CurValue() { }
    FrequencyBand(double l, double c, double h) : Lo(l), Ctr(c), Hi(h), NewValue(), CurValue() { }

    double Lo;
    double Ctr;
    double Hi;

    double NewValue;
    double CurValue;

    double Peak;        // The value of the indicator (0.0 - 1.0)
    double HoldTime;    // Time to hold the current peak value.
    double DecaySpeed;  // Speed at which the current peak value decays.
    double Opacity;     // The opacity of the indicator (0.0 - 1.0)

    WCHAR Label[16];

    D2D1_COLOR_F ForeColor;
    D2D1_COLOR_F BackColor;
};

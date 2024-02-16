
/** $VER: FrequencyBand.h (2024.02.16) P. Stuer **/

#pragma once

#include "framework.h"

struct FrequencyBand
{
    FrequencyBand() : NewValue(), CurValue(), Lo(), Ctr(), Hi(), Peak(), HoldTime(), DecaySpeed(), Opacity() { }
    FrequencyBand(double l, double c, double h) : NewValue(), CurValue(), Lo(l), Ctr(c), Hi(h), Peak(), HoldTime(), DecaySpeed(), Opacity() { }

    double NewValue;    // 0.0 .. 1.0
    double CurValue;    // 0.0 .. 1.0

    double Lo;          // Hz
    double Ctr;         // Hz
    double Hi;          // Hz

    double Peak;        // The value of the indicator (0.0 .. 1.0)
    double HoldTime;    // Time to hold the current peak value.
    double DecaySpeed;  // Speed at which the current peak value decays.
    double Opacity;     // The opacity of the indicator (0.0 .. 1.0)

    WCHAR Label[16];
    bool HasDarkBackground;
    D2D1_COLOR_F GradientColor;
};

typedef std::vector<FrequencyBand> FrequencyBands;

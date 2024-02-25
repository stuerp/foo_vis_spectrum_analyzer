
/** $VER: FrequencyBand.h (2024.02.25) P. Stuer **/

#pragma once

#include "framework.h"

struct FrequencyBand
{
    FrequencyBand() : NewValue(), CurValue(), Lo(), Ctr(), Hi(), Peak(), HoldTime(), DecaySpeed(), Opacity() { }
    FrequencyBand(double l, double c, double h) : NewValue(), CurValue(), Lo(l), Ctr(c), Hi(h), Peak(), HoldTime(), DecaySpeed(), Opacity() { }

    FrequencyBand(const FrequencyBand & other)
    {
        NewValue = other.NewValue;
        CurValue = other.CurValue;

        Lo  = other.Lo;
        Ctr = other.Ctr;
        Hi  = other.Hi;

        Peak       = other.Peak;
        HoldTime   = other.HoldTime;
        DecaySpeed = other.DecaySpeed;
        Opacity    = other.Opacity;

        ::memcpy(Label, other.Label, sizeof(Label));

        HasDarkBackground = other.HasDarkBackground;
        GradientColor     = other.GradientColor;;
    }

    virtual ~FrequencyBand() { }

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


/** $VER: FrequencyBand.h (2024.03.02) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <d2d1_2.h>

#include <vector>

#pragma warning(disable: 4820)
struct frequency_band_t
{
    frequency_band_t() : NewValue(), CurValue(), Lo(), Ctr(), Hi(), MaxValue(), HoldTime(), DecaySpeed(), Opacity() { }

    frequency_band_t(double l, double c, double h) : NewValue(), CurValue(), Lo(l), Ctr(c), Hi(h), MaxValue(), HoldTime(), DecaySpeed(), Opacity() { }

    frequency_band_t(const frequency_band_t & other)
    {
        NewValue = other.NewValue;
        CurValue = other.CurValue;

        Lo  = other.Lo;
        Ctr = other.Ctr;
        Hi  = other.Hi;

        MaxValue   = other.MaxValue;
        HoldTime   = other.HoldTime;
        DecaySpeed = other.DecaySpeed;
        Opacity    = other.Opacity;

        ::memcpy(Label, other.Label, sizeof(Label));

        HasDarkBackground = other.HasDarkBackground;
        GradientColor     = other.GradientColor;
    }

    virtual ~frequency_band_t() { }

    double NewValue;    // 0.0 .. 1.0
    double CurValue;    // 0.0 .. 1.0

    double Lo;          // Hz
    double Ctr;         // Hz
    double Hi;          // Hz

    double MaxValue;    // 0.0 .. 1.0, The value of the maximum indicator
    double HoldTime;    // Time to hold the current peak value.
    double DecaySpeed;  // Speed at which the current peak value decays.
    double Opacity;     // 0.0 .. 1.0, The opacity of the maximum indicator

    WCHAR Label[16];
    bool HasDarkBackground;
    D2D1_COLOR_F GradientColor;
};

typedef std::vector<frequency_band_t> frequency_bands_t;

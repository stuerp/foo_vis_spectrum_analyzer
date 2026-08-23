
/** $VER: FrequencyBand.h (2026.08.18) P. Stuer **/

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
    frequency_band_t() noexcept : RawValue(), Value(), Lo(), Mid(), Hi(), HoldTime(), FallRate(), MaxValue(), Opacity(1.) { }

    frequency_band_t(double l, double c, double h) noexcept : RawValue(), Value(), Lo(l), Mid(c), Hi(h), HoldTime(), FallRate(), MaxValue(), Opacity(1.) { }

    frequency_band_t(const frequency_band_t & other) noexcept
    {
        RawValue = other.RawValue;
        Value    = other.Value;

        Lo       = other.Lo;
        Mid      = other.Mid;
        Hi       = other.Hi;

        HoldTime = other.HoldTime;
        FallRate = other.FallRate;

        MaxValue = other.MaxValue;
        Opacity  = other.Opacity;

        ::memcpy(Label, other.Label, sizeof(Label));

        HasDarkBackground = other.HasDarkBackground;
        GradientColor     = other.GradientColor;
    }

    virtual ~frequency_band_t() noexcept { }

    double RawValue;    // [0, 1], Magnitude
    double Value;       // [0, 1]

    double Lo;          // Hz
    double Mid;         // Hz
    double Hi;          // Hz

    double HoldTime;    // Time to hold the current peak value (in s)
    double FallRate;    // Rate at which the current peak value decays (in dB/s)

    double MaxValue;    // [0, 1], The value of the maximum indicator
    double Opacity;     // [0, 1], The opacity of the maximum indicator

    WCHAR Label[16];
    bool HasDarkBackground;
    D2D1_COLOR_F GradientColor;
};

typedef std::vector<frequency_band_t> frequency_bands_t;

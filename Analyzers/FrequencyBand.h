
/** $VER: FrequencyBand.h (2026.09.02) P. Stuer **/

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
    frequency_band_t() noexcept : RawValue(), Value(), PeakValue(), Lo(), Mid(), Hi(), HoldTime(), FallRate(), Opacity(1.) { }

    frequency_band_t(double l, double c, double h) noexcept : RawValue(), Value(), PeakValue(), Lo(l), Mid(c), Hi(h), HoldTime(), FallRate(), Opacity(1.) { }

    frequency_band_t(const frequency_band_t & other) noexcept
    {
        RawValue  = other.RawValue;
        Value     = other.Value;
        PeakValue = other.PeakValue;

        Lo        = other.Lo;
        Mid       = other.Mid;
        Hi        = other.Hi;

        HoldTime  = other.HoldTime;
        FallRate  = other.FallRate;

        Opacity   = other.Opacity;

        ::memcpy(Label, other.Label, sizeof(Label));

        HasDarkBackground = other.HasDarkBackground;
        GradientColor     = other.GradientColor;
    }

    virtual ~frequency_band_t() noexcept { }

    double RawValue;    // Magnitude / Power
    double Value;       // [0, 1], Normalized magnitude / power
    double PeakValue;   // [0, 1], Peak normalized magnitude / power

    double Lo;          // Hz, Low frequency of this band
    double Mid;         // Hz, Center frequency of this band
    double Hi;          // Hz, High frequency of this band

    double HoldTime;    // Time to hold the current peak value (in s)
    double FallRate;    // Rate at which the current peak value decays (in dB/s)

    double Opacity;     // [0, 1], Opacity used to draw the peak normalized magnitude / power indicator

    WCHAR Label[16];
    bool HasDarkBackground;
    D2D1_COLOR_F GradientColor;

    std::vector<double> _Weights;   // Triangular Filter Bank weights for this band.
};

typedef std::vector<frequency_band_t> frequency_bands_t;

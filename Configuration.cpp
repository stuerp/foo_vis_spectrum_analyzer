
/** $VER: Configuration.cpp (2023.11.29) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"

#include <pfc/string_conv.h>
#include <pfc/string-conv-lite.h>

using namespace pfc;
using namespace stringcvt;

#include "Configuration.h"
#include "Resources.h"
#include "Math.h"

#include "Gradients.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Configuration::Configuration()
{
    Reset();
}

/// <summary>
/// Resets this instance.
/// </summary>
void Configuration::Reset() noexcept
{
    _DialogBounds = {  };

    _RefreshRateLimit = 20;

    _UseHardwareRendering = true;
    _UseAntialiasing = true;

    _UseZeroTrigger = false;
    _WindowDuration = 100;

    _Transform = Transform::FFT;

    // FFT
    _FFTSize = FFTSize::FFT4096;
    _FFTCustom = 4096;
    _FFTDuration = 100.;

    _MappingMethod = Mapping::Standard;

    // Frequencies
    _FrequencyDistribution = FrequencyDistribution::Octaves;

    // Frequency range
    _NumBands = 320;
    _LoFrequency = 20.;
    _HiFrequency = 20000.;

    // Note range
    _MinNote = 0;
    _MaxNote = 143;
    _BandsPerOctave = 12;
    _Pitch = 440.0;
    _Transpose = 0;

    // Frequencies
    _ScalingFunction = ScalingFunction::Logarithmic;

    _SkewFactor = 0.0;
    _Bandwidth = 0.5;

    _SmoothingMethod = SmoothingMethod::Peak;
    _SmoothingFactor = 0.5;

    _KernelSize = 32;
    _SummationMethod = SummationMethod::Maximum;
    _SmoothLowerFrequencies = true;
    _SmoothGainTransition = true;

    // Rendering parameters
    _BackColor = D2D1::ColorF(0.f, 0.f, 0.f, 1.f);

    // X axis
    _XAxisMode = XAxisMode::Notes;

    _XTextColor = D2D1::ColorF(D2D1::ColorF::White);
    _XLineColor = D2D1::ColorF(.25f, .25f, .25f, 1.f);

    // Y axis
    _YAxisMode = YAxisMode::Decibels;

    _YTextColor = D2D1::ColorF(D2D1::ColorF::White);
    _YLineColor = D2D1::ColorF(.25f, .25f, .25f, 1.f);

    _AmplitudeLo = -90.;
    _AmplitudeHi =   0.;
    _AmplitudeStep = -6.;

    _UseAbsolute = true;
    _Gamma = 1.;

    // Band
    _DrawBandBackground = true;
    _BandBackColor = D2D1::ColorF(.2f, .2f, .2f, .7f);

    _ColorScheme = ColorScheme::Prism1;

    _GradientStops = GetGradientStops(_ColorScheme);
    _CustomGradientStops = GetGradientStops(ColorScheme::Custom);

    _PeakMode = PeakMode::Classic;
    _HoldTime = 30.;
    _Acceleration = 0.5;

    // Logging
    _LogLevel = LogLevel::None;
/*
    bandwidthOffset: 1,

    windowFunction: 'hann',
    windowParameter: 1,
    windowSkew: 0,

    timeAlignment: 1,
    downsample: 0,

    color: 'none',

    showLabels: true,
    showLabelsY: true,

    labelTuning: 440,

    showDC: true,
    showNyquist: true,

    mirrorLabels: true,
    diffLabels: false,
    compensateDelay: false
*/
}

/// <summary>
/// Implements the = operator.
/// </summary>
Configuration & Configuration::operator=(const Configuration & other)
{
    _DialogBounds = other._DialogBounds;

    _RefreshRateLimit = other._RefreshRateLimit;

    _UseHardwareRendering = other._UseHardwareRendering;
    _UseAntialiasing = other._UseAntialiasing;

    _UseZeroTrigger = other._UseZeroTrigger;
    _WindowDuration = other._WindowDuration;

    // Transform type
    _Transform = other._Transform;

    #pragma region FFT
        _FFTSize = other._FFTSize;
        _FFTCustom = other._FFTCustom;
        _FFTDuration = other._FFTDuration;

        _MappingMethod = other._MappingMethod;
        _SmoothingMethod = other._SmoothingMethod;
        _SmoothingFactor = other._SmoothingFactor;
        _KernelSize = other._KernelSize;
        _SummationMethod = other._SummationMethod;
        _SmoothLowerFrequencies = other._SmoothLowerFrequencies;
        _SmoothGainTransition = other._SmoothGainTransition;
    #pragma endregion

    #pragma region Frequencies
        _FrequencyDistribution = other._FrequencyDistribution;

        _NumBands = other._NumBands;
        _LoFrequency = other._LoFrequency;
        _HiFrequency = other._HiFrequency;

        // Note range
        _MinNote = other._MinNote;
        _MaxNote = other._MaxNote;
        _BandsPerOctave = other._BandsPerOctave;
        _Pitch = other._Pitch;
        _Transpose = other._Transpose;

        _ScalingFunction = other._ScalingFunction;
        _SkewFactor = other._SkewFactor;
        _Bandwidth = other._Bandwidth;
    #pragma endregion

    #pragma region Rendering
        _BackColor = other._BackColor;

        // X axis
        _XAxisMode = other._XAxisMode;

        _XTextColor = other._XTextColor;
        _XLineColor = other._XLineColor;

        // Y axis
        _YAxisMode = other._YAxisMode;

        _YTextColor = other._YTextColor;
        _YLineColor = other._YLineColor;

        _AmplitudeLo = other._AmplitudeLo;
        _AmplitudeHi = other._AmplitudeHi;
        _AmplitudeStep = other._AmplitudeStep;

        _UseAbsolute = other._UseAbsolute;

        _Gamma = other._Gamma;

        // Bands
        _DrawBandBackground = other._DrawBandBackground;
        _BandBackColor = other._BandBackColor;

        _ColorScheme = other._ColorScheme;

        _GradientStops = other._GradientStops;
        _CustomGradientStops = other._CustomGradientStops;

        _PeakMode = other._PeakMode;
        _HoldTime = other._HoldTime;
        _Acceleration = other._Acceleration;
    #pragma endregion

    // Logging
    _LogLevel = other._LogLevel;

    return *this;
}

/// <summary>
/// Reads this instance from the specified parser.
/// </summary>
void Configuration::Read(ui_element_config_parser & parser)
{
    Reset();

    size_t Version;

    parser >> Version;

    if (Version > _CurrentVersion)
        return;

    int Integer;

    parser >> _DialogBounds.left;
    parser >> _DialogBounds.top;
    parser >> _DialogBounds.right;
    parser >> _DialogBounds.bottom;

    parser >> _RefreshRateLimit; _RefreshRateLimit = Clamp<size_t>(_RefreshRateLimit, 20, 200);

    parser >> _UseHardwareRendering;
    parser >> _UseAntialiasing;

    parser >> _UseZeroTrigger;

    parser >> _WindowDuration; _WindowDuration = Clamp<size_t>(_WindowDuration, 50, 800);

    parser >> Integer; _Transform = (Transform) Integer;

#pragma region FFT
    parser >> Integer; _FFTSize = (FFTSize) Integer;
    parser >> _FFTCustom;
    parser >> _FFTDuration;

    parser >> Integer; _MappingMethod = (Mapping) Integer;
    parser >> Integer; _SmoothingMethod = (SmoothingMethod) Integer;
    parser >> _SmoothingFactor;
    parser >> _KernelSize;
    parser >> Integer; _SummationMethod = (SummationMethod) Integer;
    parser >> _SmoothLowerFrequencies;
    parser >> _SmoothGainTransition;
#pragma endregion

#pragma region Frequencies
    parser >> Integer; _FrequencyDistribution = (FrequencyDistribution) Integer;

    parser >> _NumBands;

    if (Version < 5)
    {
        parser >> Integer; _LoFrequency = Integer; // In v5 _LoFrequency became double
        parser >> Integer; _HiFrequency = Integer; // In v5 _LoFrequency became double
    }
    else
    {
        parser >> _LoFrequency;
        parser >> _HiFrequency;
    }

    parser >> _MinNote;
    parser >> _MaxNote;
    parser >> _BandsPerOctave;
    parser >> _Pitch;
    parser >> _Transpose;

    parser >> Integer; _ScalingFunction = (ScalingFunction) Integer;
    parser >> _SkewFactor;
    parser >> _Bandwidth;
#pragma endregion

#pragma region Rendering
    if (Version < 5)
    {
        UINT32 Rgb; parser >> Rgb;
        FLOAT Alpha; parser >> Alpha;

        _BackColor = D2D1::ColorF(Rgb, Alpha);
    }
    else
    {
        parser >> _BackColor.r;
        parser >> _BackColor.g;
        parser >> _BackColor.b;
        parser >> _BackColor.a;
    }

    parser >> Integer; _XAxisMode = (XAxisMode) Integer;

    parser >> Integer; _YAxisMode = (YAxisMode) Integer;

    parser >> _AmplitudeLo;
    parser >> _AmplitudeHi;
    parser >> _UseAbsolute;
    parser >> _Gamma;

    parser >> Integer; _ColorScheme = (ColorScheme) Integer;

    parser >> _DrawBandBackground;

    parser >> Integer; _PeakMode = (PeakMode) Integer;
    parser >> _HoldTime;
    parser >> _Acceleration;
#pragma endregion

    // Version 5
    if (Version >= 5)
    {
        _CustomGradientStops.clear();

        size_t Count; parser >> Count;

        for (size_t i = 0; i < Count; ++i)
        {
            D2D1_GRADIENT_STOP gs = { };

            parser >> gs.position;
            parser >> gs.color.r;
            parser >> gs.color.g;
            parser >> gs.color.b;
            parser >> gs.color.a;

            _CustomGradientStops.push_back(gs);
        }
    }

    parser >> _XTextColor.r;
    parser >> _XTextColor.g;
    parser >> _XTextColor.b;
    parser >> _XTextColor.a;

    parser >> _XLineColor.r;
    parser >> _XLineColor.g;
    parser >> _XLineColor.b;
    parser >> _XLineColor.a;

    parser >> _YTextColor.r;
    parser >> _YTextColor.g;
    parser >> _YTextColor.b;
    parser >> _YTextColor.a;

    parser >> _YLineColor.r;
    parser >> _YLineColor.g;
    parser >> _YLineColor.b;
    parser >> _YLineColor.a;

    parser >> _BandBackColor.r;
    parser >> _BandBackColor.g;
    parser >> _BandBackColor.b;
    parser >> _BandBackColor.a;

    // Version 6
    if (Version >= 6)
        parser >> _AmplitudeStep;

    if (_ColorScheme != ColorScheme::Custom)
        _GradientStops = GetGradientStops(_ColorScheme);
    else
        _GradientStops = _CustomGradientStops;
}

/// <summary>
/// Writes this instance to the specified builder.
/// </summary>
void Configuration::Write(ui_element_config_builder & builder) const
{
    builder << _CurrentVersion;

    #pragma region User Interface
        builder << _DialogBounds.left;
        builder << _DialogBounds.top;
        builder << _DialogBounds.right;
        builder << _DialogBounds.bottom;

        builder << _RefreshRateLimit;

        builder << _UseHardwareRendering;
        builder << _UseAntialiasing;

        builder << _UseZeroTrigger;
        builder << _WindowDuration;
    #pragma endregion

        builder << (int) _Transform;

    #pragma region FFT
        builder << (int) _FFTSize;
        builder << _FFTCustom;
        builder << _FFTDuration;
        builder << (int) _MappingMethod;

        builder << (int) _SmoothingMethod;
        builder << _SmoothingFactor;
        builder << _KernelSize;
        builder << (int) _SummationMethod;
        builder << _SmoothLowerFrequencies;
        builder << _SmoothGainTransition;
    #pragma endregion

    #pragma region Frequencies
        builder << (int) _FrequencyDistribution;

        builder << _NumBands;
        builder << _LoFrequency;
        builder << _HiFrequency;

        builder << _MinNote;
        builder << _MaxNote;
        builder << _BandsPerOctave;
        builder << _Pitch;
        builder << _Transpose;

        builder << (int) _ScalingFunction;
        builder << _SkewFactor;
        builder << _Bandwidth;
    #pragma endregion

    #pragma region Rendering
        builder << _BackColor.r;
        builder << _BackColor.g;
        builder << _BackColor.b;
        builder << _BackColor.a;

        builder << (int) _XAxisMode;

        builder << (int) _YAxisMode;

        builder << _AmplitudeLo;
        builder << _AmplitudeHi;
        builder << _UseAbsolute;
        builder << _Gamma;

        builder << (int) _ColorScheme;

        builder << _DrawBandBackground;

        builder << (int) _PeakMode;
        builder << _HoldTime;
        builder << _Acceleration;
    #pragma endregion

    // Version 5
    builder << _CustomGradientStops.size();

    for (const auto & Iter : _CustomGradientStops)
    {
        builder << Iter.position;
        builder << Iter.color.r;
        builder << Iter.color.g;
        builder << Iter.color.b;
        builder << Iter.color.a;
    }

    builder << _XTextColor.r;
    builder << _XTextColor.g;
    builder << _XTextColor.b;
    builder << _XTextColor.a;

    builder << _XLineColor.r;
    builder << _XLineColor.g;
    builder << _XLineColor.b;
    builder << _XLineColor.a;

    builder << _YTextColor.r;
    builder << _YTextColor.g;
    builder << _YTextColor.b;
    builder << _YTextColor.a;

    builder << _YLineColor.r;
    builder << _YLineColor.g;
    builder << _YLineColor.b;
    builder << _YLineColor.a;

    builder << _BandBackColor.r;
    builder << _BandBackColor.g;
    builder << _BandBackColor.b;
    builder << _BandBackColor.a;

    // Version 6
    builder << _AmplitudeStep;
}

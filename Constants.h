
/** $VER: Constans.h (2024.04.16) P. Stuer **/

#pragma once

#include <stdint.h>

inline const int MinFFTSize =     2;
inline const int MaxFFTSize = 32768;

inline const double MinFFTDuration =    1.; // ms
inline const double MaxFFTDuration = 2000.; // ms

inline const int MinKernelSize =  1;
inline const int MaxKernelSize = 64;

// Window Function / Brown-Puckette CQT Kernel
inline const double MinWindowParameter =  0.;
inline const double MaxWindowParameter = 10.;

inline const double MinWindowSkew = -1.;
inline const double MaxWindowSkew =  1.;

inline const double MinReactionAlignment = -0.5;
inline const double MaxReactionAlignment =  0.5;

// Brown-Puckette CQT
inline const double MinBandwidthOffset = 0.;
inline const double MaxBandwidthOffset = 1.;

inline const double MinBandwidthCap = 0.;
inline const double MaxBandwidthCap = 1.;

inline const double MinBandwidthAmount =   0.;
inline const double MaxBandwidthAmount = 256.;

// SWIFT
inline const size_t MinFilterBankOrder = 1;
inline const size_t MaxFilterBankOrder = 8;

inline const double MinTimeResolution = 0.; 
inline const double MaxTimeResolution = 2000.;

inline const double MinIIRBandwidth =  0.;
inline const double MaxIIRBandwidth = 64.;

// Analog-style (parallel band-pass IIR filter) transform

// Frequencies
inline const int MinBands =   2;
inline const int MaxBands = 512;

inline const double MinFrequency =     1.; // Hz
inline const double MaxFrequency = 96000.; // Hz

inline const int MinNote =   0;
inline const int MaxNote = 143;

inline const int MinBandsPerOctave =  1;
inline const int MaxBandsPerOctave = 48;

inline const double MinPitch =    16.35; // Hz, C0
inline const double MaxPitch = 63217.06; // Hz, B11

inline const int MinTranspose = -24;
inline const int MaxTranspose =  24;

inline const double MinSkewFactor = 0.;
inline const double MaxSkewFactor = 1.;

inline const double MinBandwidth =  0.;
inline const double MaxBandwidth = 64.;

inline const double MinAmplitude = -120.; // dB
inline const double MaxAmplitude =    6.; // dB

inline const double MinAmplitudeStep = -10.; // dB
inline const double MaxAmplitudeStep =  -1.; // dB

inline const double MinGamma =  0.5;
inline const double MaxGamma = 10.0;



// Filters
inline const double MinSlopeFunctionOffset = 0.;
inline const double MaxSlopeFunctionOffset = 8.;

inline const double MinSlope = -12.;
inline const double MaxSlope =  12.;

inline const double MinSlopeOffset =     0.; // Hz
inline const double MaxSlopeOffset = 96000.; // Hz

inline const double MinEqualizeAmount = -12.;
inline const double MaxEqualizeAmount =  12.;

inline const double MinEqualizeOffset =     0.; // Hz
inline const double MaxEqualizeOffset = 96000.; // Hz

inline const double MinEqualizeDepth =     0.; // Hz
inline const double MaxEqualizeDepth = 96000.; // Hz

inline const double MinWeightingAmount = -1.; // %
inline const double MaxWeightingAmount =  1.; // %



inline const double MinSmoothingFactor = 0.;
inline const double MaxSmoothingFactor = 1.;

inline const double MinHoldTime =   0.;
inline const double MaxHoldTime = 120.;

inline const double MinAcceleration = 0.;
inline const double MaxAcceleration = 2.;

inline const FLOAT MinLEDSize =  0.f;
inline const FLOAT MaxLEDSize = 32.f;

inline const FLOAT MinLEDGap =  0.f;
inline const FLOAT MaxLEDGap = 32.f;

inline const double MinRMSWindow = 0.; // in seconds
inline const double MaxRMSWindow = 3.; // in seconds

inline const FLOAT MinGaugeGap =   0.; // in pixels
inline const FLOAT MaxGaugeGap = 100.; // in pixels

inline const double MinArtworkOpacity = 0.;
inline const double MaxArtworkOpacity = 1.;

inline const uint32_t MinArtworkColors = 2;
inline const uint32_t MaxArtworkColors = 16;

inline const double MinLightnessThreshold = 0.;
inline const double MaxLightnessThreshold = 1.;



inline const double MinOpacity = 0.;
inline const double MaxOpacity = 1.;

inline const double MinThickness =  0.;
inline const double MaxThickness = 32.;

inline const double MinFontSize = 1.;
inline const double MaxFontSize = 200.;



enum class Transform
{
    FFT = 0,
    CQT = 1,
    SWIFT = 2,
    AnalogStyle = 3,
};

enum class FFTMode
{
    FFT64       = 0,
    FFT128      = 1,
    FFT256      = 2,
    FFT512      = 3,
    FFT1024     = 4,
    FFT2048     = 5,
    FFT4096     = 6,
    FFT8192     = 7,
    FFT16384    = 8,
    FFT32768    = 9,
    FFT65536    = 10,

    FFTCustom   = 11,
    FFTDuration = 12,
};

enum class Mapping
{
    Standard = 0,
    TriangularFilterBank = 1,
    BrownPuckette = 2,
};

enum class FrequencyDistribution
{
    Linear = 0,
    Octaves = 1,
    AveePlayer = 2,
};

enum class WeightingType
{
    None = 0,

    AWeighting = 1, // A-weighting. https://en.wikipedia.org/wiki/A-weighting
    BWeighting = 2,
    CWeighting = 3,
    DWeighting = 4,

    MWeighting = 5, // M-weighting, related to ITU-R 468 noise weighting, https://en.wikipedia.org/wiki/ITU-R_468_noise_weighting
};

enum class ScalingFunction
{
    Linear = 0,

    Logarithmic = 1,
    ShiftedLogarithmic = 2,

    Mel = 3, // AIMP

    Bark = 4,
    AdjustableBark = 5,

    ERB = 6,
    Cams = 7,
    HyperbolicSine = 8,
    NthRoot = 9,
    NegativeExponential = 10,
    Period = 11,
};

enum class SummationMethod
{
    Minimum = 0,
    Maximum = 1,

    Sum = 2,

    RMS = 3,
    RMSSum = 4,

    Average = 5,
    Median = 6
};

enum class SmoothingMethod
{
    None = 0,

    Average = 1,
    Peak = 2,
};

enum class XAxisMode
{
    None = 0,

    Bands = 1,
    Decades = 2,
    Octaves = 3,
    Notes = 4,
};

enum class YAxisMode
{
    None = 0,

    Decibels = 1,
    Linear = 2,
};

enum class VisualizationType
{
    Bars = 0,
    Curve = 1,
    Spectogram = 2,
    PeakMeter = 3,
};

enum class PeakMode
{
    None = 0,

    Classic = 1,
    Gravity = 2,
    AIMP = 3,
    FadeOut = 4,
    FadingAIMP = 5,
};

enum class BackgroundMode
{
    None = 0,

    Solid = 1,
    Artwork = 2,
};

enum class ColorOrder
{
    None = 0,

    HueAscending = 1,
    HueDescending = 2,

    SaturationAscending = 3,
    SaturationDescending = 4,

    LightnessAscending = 5,
    LightnessDescending = 6,
};

enum class FitMode
{
    Free = 0,

    FitBig,
    FitWidth,
    FitHeight,

    Fill,
};

enum class VisualElement : uint32_t
{
    GraphBackground             =  0,
    GraphDescriptionText        =  1,
    GraphDescriptionBackground  = 14,

    XAxisText                   =  2,
    VerticalGridLine            =  3,
    YAxisText                   =  4,
    HorizontalGridLine          =  5,

    BarArea                     =  6,
    BarTop                      = 16,
    BarPeakTop                  =  7,
    BarPeakArea                 = 17,
    BarDarkBackground           =  8,
    BarLightBackground          =  9,

    CurveLine                   = 10,
    CurveArea                   = 11,
    CurvePeakLine               = 12,
    CurvePeakArea               = 13,

    Spectogram                  = 18,

    GaugeBackground         = 19,
    GaugePeakLevel          = 20,
    Gauge0dBPeakLevel       = 23,
    GaugeMaxPeakLevel       = 25,

    GaugeRMSLevel           = 21,
    Gauge0dBRMSLevel        = 24,
    GaugeRMSLevelText       = 22,

    NyquistMarker               = 15,

    Count                       = 26
};

enum class ColorSource : uint32_t
{
    None,
    Solid,
    DominantColor,
    Gradient,
    Windows,
    UserInterface,
};

enum class WindowsColor : uint32_t
{
    WindowBackground,           // COLOR_WINDOW
    WindowText,                 // COLOR_WINDOWTEXT
    ButtonBackground,           // COLOR_3DFACE
    ButtonText,                 // COLOR_BTNTEXT
    HighlightBackground,        // COLOR_HIGHLIGHT
    HighlightText,              // COLOR_HIGHLIGHTTEXT
    GrayText,                   // COLOR_GRAYTEXT
    HotLight,                   // COLOR_HOTLIGHT
};

enum class DUIColor : uint32_t
{
    Text,
    Background,
    Highlight,
    Selection,
    DarkMode
};

enum class CUIColor : uint32_t
{
    Text,                       // cui::colours::colour_text
    SelectedText,               // cui::colours::colour_selection_text
    InactiveSelectedText,       // cui::colours::colour_inactive_selection_text

    Background,                 // cui::colours::colour_background
    SelectedBackground,         // cui::colours::colour_selection_background
    InactiveSelectedBackground, // cui::colours::colour_inactive_selection_background

    ActiveItem,                 // cui::colours::colour_active_item_frame
};

enum class ColorScheme : uint32_t
{
    Solid = 0,
    Custom = 1,
    Artwork = 2,

    Prism1 = 3,
    Prism2 = 4,
    Prism3 = 5,

    foobar2000 = 6,
    foobar2000DarkMode = 7,

    Fire = 8,
    Rainbow = 9,

    SoX = 10,
};

// Should be the exact layout as in "sdk/audio_chunk.h". No need to include foobar2000 SDK everywhere.
enum class Channel : uint32_t
{
    FrontLeft = 1 << 0,
    FrontRight = 1 << 1,
    FrontCenter = 1 << 2,

    LFE = 1 << 3,
    BackLeft = 1 << 4,
    BackRight = 1 << 5,

    FrontCenterLeft = 1 << 6,
    FrontCenterRight = 1 << 7,
    BackCenter = 1 << 8,

    SideLeft = 1 << 9,
    SideRight = 1 << 10,

    TopCenter = 1 << 11,
    TopFrontLeft = 1 << 12,
    TopFrontCenter = 1 << 13,
    TopFrontRight = 1 << 14,
    TopBackLeft = 1 << 15,
    TopBackCenter = 1 << 16,
    TopBackRight = 1 << 17,

    BackLeftRight = BackLeft | BackRight,
    SideLeftRight = SideLeft | SideRight,

    ConfigMono          = FrontCenter,
    ConfigStereo        = FrontLeft | FrontRight,
    Config2point1       = ConfigStereo | LFE,
    Config3point0       = ConfigStereo | FrontCenter,
    Config4point0       = ConfigStereo | BackLeftRight,
    Config4point0Side   = ConfigStereo | SideLeftRight,
    Config4point1       = Config4point0 | LFE,
    Config5point0       = Config4point0 | FrontCenter,
    Config6point0       = Config4point0 | SideLeftRight,
    Config5point1       = Config4point0 | FrontCenter | LFE,
    Config5point1Side   = Config4point0Side | FrontCenter | LFE,
    Config7point1       = Config5point1 | SideLeftRight,

    Count = 18,
};

inline const uint32_t AllChannels = ((1 << (uint32_t) Channel::Count) - 1);

enum class HorizontalAlignment : uint32_t
{
    Left = 0,
    Center = 1,
    Right = 2
};

enum class VerticalAlignment : uint32_t
{
    Top = 0,
    Center = 1,
    Bottom = 2
};

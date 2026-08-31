
/** $VER: Constants.h (2026.08.31) P. Stuer **/

#pragma once

#include <stdint.h>

enum class VisualizationType
{
    Bars = 0,
    Curve = 1,
    Spectrogram = 2,
    PeakMeter = 3,
    LevelMeter = 4,
    RadialBars = 5,
    RadialCurve = 6,
    Oscilloscope = 7,
    BitMeter = 8,

    Tester = 63,

    Min = Bars,
    Max = Tester,
};

// Common
inline constexpr int64_t MinRefreshRate = 20;
inline constexpr int64_t MaxRefreshRate = 200;

inline constexpr double MinSmoothingFactor = 0.;
inline constexpr double MaxSmoothingFactor = 1.;

inline constexpr double MinArtworkOpacity = 0.;
inline constexpr double MaxArtworkOpacity = 1.;

inline constexpr double MinArtworkBlurSigma = 0.;
inline constexpr double MaxArtworkBlurSigma = 20.;

inline constexpr uint32_t MinArtworkColors = 2;
inline constexpr uint32_t MaxArtworkColors = 16;

inline constexpr double MinLightnessThreshold = 0.;
inline constexpr double MaxLightnessThreshold = 1.;

enum class SmoothingMethod
{
    None = 0,

    Average = 1,
    Peak = 2,

    Min = None,
    Max = Peak,
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

    Min = None,
    Max = LightnessDescending,
};

enum class ArtworkType
{
    Front = 0,
    Back,
    Disc,
    Icon,
    Artist,

    Min = Front,
    Max = Artist,
};

enum class FitMode
{
    Free = 0,

    FitBig,
    FitWidth,
    FitHeight,

    Fill,

    Min = Free,
    Max = Fill,
};

// Visualization
inline constexpr double MinHoldTime =   0.; // s
inline constexpr double MaxHoldTime =   5.; // s

inline constexpr double MinFallRate =   0.; // dB/s
inline constexpr double MaxFallRate = 120.; // dB/s

inline constexpr FLOAT MinLEDSize =  0.f;   // DIPs
inline constexpr FLOAT MaxLEDSize = 32.f;   // DIPs

inline constexpr FLOAT MinLEDGap =  0.f;    // DIPs
inline constexpr FLOAT MaxLEDGap = 32.f;    // DIPs

inline constexpr double MinRMSWindow = 0.;  // s
inline constexpr double MaxRMSWindow = 3.;  // s

inline constexpr FLOAT MinBarGap =   0.;    // DIPs
inline constexpr FLOAT MaxBarGap = std::numeric_limits<FLOAT>::max(); // DIPs

inline constexpr FLOAT MinBarSize =   0.;   // DIPs
inline constexpr FLOAT MaxBarSize = std::numeric_limits<FLOAT>::max(); // DIPs

enum class PeakMode
{
    None = 0,

    Classic = 1,
    Gravity = 2,
    AIMP = 3,
    FadeOut = 4,
    FadingAIMP = 5,

    Min = None,
    Max = FadingAIMP,
};

// Transform
inline constexpr int MinFFTSize =     2;
inline constexpr int MaxFFTSize = 32768;

inline constexpr double MinFFTDuration =    1.; // ms
inline constexpr double MaxFFTDuration = 2000.; // ms

inline constexpr int MinKernelSize =  1;
inline constexpr int MaxKernelSize = 64;

enum class TransformMethod
{
    FFT = 0,
    CQT = 1,
    SWIFT = 2,
    AnalogStyle = 3,

    Min = FFT,
    Max = AnalogStyle,
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

    Min         = FFT64,
    Max         = FFTDuration,
};

enum class Mapping
{
    Standard = 0,
    TriangularFilterBank = 1,
    BrownPuckette = 2,

    Min = Standard,
    Max = BrownPuckette,
};

enum class AggregationMethod
{
    Minimum = 0,
    Maximum = 1,

    Sum = 2,

    RMS = 3,
    RMSSum = 4,

    Average = 5,
    Median = 6,

    Min = Minimum,
    Max = Median,
};

// Window Function / Brown-Puckette CQT Kernel
inline constexpr double MinWindowParameter =  0.;
inline constexpr double MaxWindowParameter = 10.;

inline constexpr double MinWindowSkew = -1.;
inline constexpr double MaxWindowSkew =  1.;

inline constexpr double MinReactionAlignment = -0.5;
inline constexpr double MaxReactionAlignment =  0.5;

// Brown-Puckette CQT
inline constexpr double MinBandwidthOffset = 0.;
inline constexpr double MaxBandwidthOffset = 1.;

inline constexpr double MinBandwidthCap = 0.;
inline constexpr double MaxBandwidthCap = 1.;

inline constexpr double MinBandwidthAmount =   0.;
inline constexpr double MaxBandwidthAmount = 256.;

// SWIFT
inline constexpr size_t MinFilterBankOrder = 1;
inline constexpr size_t MaxFilterBankOrder = 8;

inline constexpr double MinTimeResolution = 0.; 
inline constexpr double MaxTimeResolution = 2000.;

inline constexpr double MinIIRBandwidth =  0.;
inline constexpr double MaxIIRBandwidth = 64.;

// Analog-style (parallel band-pass IIR filter) transform

// Frequencies
inline constexpr int MinBands =    2;
inline constexpr int MaxBands = 8192;

inline constexpr double MinFrequency =     1.; // Hz
inline constexpr double MaxFrequency = 96000.; // Hz

inline constexpr int MinNote =   0;
inline constexpr int MaxNote = 143;

inline constexpr int MinBandsPerOctave =  1;
inline constexpr int MaxBandsPerOctave = 48;

inline constexpr double MinPitch =    16.35; // Hz, C0
inline constexpr double MaxPitch = 63217.06; // Hz, B11

inline constexpr int MinTranspose = -24;
inline constexpr int MaxTranspose =  24;

inline constexpr double MinSkewFactor = 0.;
inline constexpr double MaxSkewFactor = 1.;

inline constexpr double MinBandwidth =  0.;
inline constexpr double MaxBandwidth = 64.;

inline constexpr double MinAmplitude = -120.; // dB
inline constexpr double MaxAmplitude =    6.; // dB

inline constexpr double MinAmplitudeStep = -10.; // dB
inline constexpr double MaxAmplitudeStep =  -1.; // dB

inline constexpr double MinGamma =  0.5;
inline constexpr double MaxGamma = 10.0;

inline constexpr int MinXAxisDecimals = 0;
inline constexpr int MaxXAxisDecimals = 3;

inline constexpr int MinMelBands =   24;
inline constexpr int MaxMelBands =  128;

enum class FrequencyDistribution
{
    Linear = 0,
    Octaves = 1,
    AveePlayer = 2,
    Mel = 3,

    Min = Linear,
    Max = Mel,
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

    Min = Linear,
    Max = Period,
};

// Filters
inline constexpr double MinSlopeFunctionOffset = 0.;
inline constexpr double MaxSlopeFunctionOffset = 8.;

inline constexpr double MinSlope = -12.;
inline constexpr double MaxSlope =  12.;

inline constexpr double MinSlopeOffset =     0.; // Hz
inline constexpr double MaxSlopeOffset = 96000.; // Hz

inline constexpr double MinEqualizeAmount = -12.;
inline constexpr double MaxEqualizeAmount =  12.;

inline constexpr double MinEqualizeOffset =     0.; // Hz
inline constexpr double MaxEqualizeOffset = 96000.; // Hz

inline constexpr double MinEqualizeDepth =     0.; // Hz
inline constexpr double MaxEqualizeDepth = 96000.; // Hz

inline constexpr double MinWeightingAmount = -1.; // %
inline constexpr double MaxWeightingAmount =  1.; // %

enum class WeightingType
{
    None = 0,

    AWeighting = 1, // A-weighting. https://en.wikipedia.org/wiki/A-weighting
    BWeighting = 2,
    CWeighting = 3,
    DWeighting = 4,

    MWeighting = 5, // M-weighting, related to ITU-R 468 noise weighting, https://en.wikipedia.org/wiki/ITU-R_468_noise_weighting

    Min = None,
    Max = MWeighting,
};

// Styles
inline constexpr double MinOpacity = 0.;
inline constexpr double MaxOpacity = 1.;

inline constexpr double MinThickness =  0.;
inline constexpr double MaxThickness = 32.;

inline constexpr double MinFontSize = 1.;
inline constexpr double MaxFontSize = 200.;

enum class VisualElement : uint32_t
{
    GraphBackground             =  0,
    GraphDescriptionText        =  1,
    GraphDescriptionBackground  = 14,

    XAxisText                   =  2,
    XAxisLine                   = 33,
    YAxisText                   =  4,
    YAxisLine                   = 34,

    HorizontalGridLine          =  5,
    VerticalGridLine            =  3,

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

    Spectrogram                 = 18,

    BarBackground               = 19,

    BarPeakLevel                = 20,
    Bar0dBPeakLevel             = 23,
    BarMaxPeakLevel             = 25,
    BarPeakLevelText            = 26,

    BarRMSLevel                 = 21,
    Bar0dBRMSLevel              = 24,
    BarRMSLevelText             = 22,

    NyquistMarker               = 15,

    BarLeftRight                = 27,
    BarMidSide                  = 28,
    LevelMeterAxis              = 29,
    BarLeftRightIndicator       = 30,
    BarMidSideIndicator         = 31,

    SignalLine                  = 32,

    BarSign                     = 35,
    BarMantissa                 = 36,
    BarExponent                 = 37,

    Count                       = 38
};

enum class ColorSource : uint32_t
{
    None = 0,
    Solid,
    DominantColor,
    Gradient,
    Windows,
    UserInterface,

    Min = None,
    Max = UserInterface,
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

    Prism1,
    Prism2,
    Prism3,

    foobar2000,
    foobar2000DarkMode,

    Fire,
    Rainbow,

    SoX,

    Turbo,

    Viridis,
    Plasma,
    Inferno,
    Magma,
    Cividis,

    Min = Solid,
    Max = Cividis,
};

enum class VisualizationTypes : uint64_t
{
    None = 0ull,

    Bars            = 1 << (int) VisualizationType::Bars,
    Curve           = 1 << (int) VisualizationType::Curve,
    Spectrogram     = 1 << (int) VisualizationType::Spectrogram,
    PeakMeter       = 1 << (int) VisualizationType::PeakMeter,
    LevelMeter      = 1 << (int) VisualizationType::LevelMeter,
    RadialBars      = 1 << (int) VisualizationType::RadialBars,
    RadialCurve     = 1 << (int) VisualizationType::RadialCurve,
    Oscilloscope    = 1 << (int) VisualizationType::Oscilloscope,
    BitMeter        = 1 << (int) VisualizationType::BitMeter,

    All = ~0ull
};

// Oscilloscope
inline constexpr double MinXGain =  0.;
inline constexpr double MaxXGain = 10.;

inline constexpr double MinYGain =  0.;
inline constexpr double MaxYGain = 10.;

inline constexpr FLOAT MinRotation = -180.f;
inline constexpr FLOAT MaxRotation =  180.f;

inline constexpr uint32_t MinFrameCount = 256;
inline constexpr uint32_t MaxFrameCount = std::numeric_limits<uint32_t>::max();

inline constexpr FLOAT MinBlurSigma =  1.f;
inline constexpr FLOAT MaxBlurSigma = 10.f;

inline constexpr FLOAT MinDecayFactor = 0.f;
inline constexpr FLOAT MaxDecayFactor = 1.f;

// Bit Meter
enum class BitMeterMode : uint32_t
{
    FloatingPoint,
    Integer,

    Min = FloatingPoint,
    Max = Integer,
};

inline constexpr uint8_t MinBitsPerInteger =  1;
inline constexpr uint8_t MaxBitsPerInteger = 32;

// Graphs
enum class HorizontalAlignment
{
    Near = 0,
    Center,
    Far,
    Fit,

    Min = Near,
    Max = Fit
};

enum class VerticalAlignment
{
    Near = 0,
    Center,
    Far,

    Min = Near,
    Max = Far
};

enum class XAxisMode
{
    None = 0,

    Bands = 1,
    Decades = 2,
    Octaves = 3,
    Notes = 4,

    Min = None,
    Max = Notes,
};

enum class YAxisMode
{
    None = 0,

    Decibels = 1,
    Linear = 2,

    Min = None,
    Max = Linear,
};

// Deprecated
enum class BackgroundMode
{
    None = 0,

    Solid = 1,
    Artwork = 2,

    Min = None,
    Max = Artwork,
};

// Should be the exact layout as in "sdk/audio_chunk.h". No need to include foobar2000 SDK everywhere.
enum class Channels : uint32_t
{
    FrontLeft           = 1 <<  0,
    FrontRight          = 1 <<  1,
    FrontCenter         = 1 <<  2,

    LFE                 = 1 <<  3,

    BackLeft            = 1 <<  4,
    BackRight           = 1 <<  5,

    FrontCenterLeft     = 1 <<  6,
    FrontCenterRight    = 1 <<  7,

    BackCenter          = 1 <<  8,

    SideLeft            = 1 <<  9,
    SideRight           = 1 << 10,

    TopCenter           = 1 << 11,

    TopFrontLeft        = 1 << 12,
    TopFrontCenter      = 1 << 13,
    TopFrontRight       = 1 << 14,

    TopBackLeft         = 1 << 15,
    TopBackCenter       = 1 << 16,
    TopBackRight        = 1 << 17,

    BackLeftRight       = BackLeft | BackRight,
    SideLeftRight       = SideLeft | SideRight,

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

    Count = 18u,

    None = 0,
    All = (1 << Count) - 1,
};

inline constexpr uint32_t AllChannels = ((1 << (uint32_t) Channels::Count) - 1);

enum class ChannelPair : uint32_t
{
    FrontLeftRight = 0,
    BackLeftRight,

    FrontCenterLeftRight,
    SideLeftRight,

    TopFrontLeftRight,
    TopBackLeftRight,

    Min = FrontLeftRight,
    Max = TopBackLeftRight,
};

enum class HorizontalTextAlignment : uint32_t
{
    Left = 0,
    Center = 1,
    Right = 2,

    Min = Left,
    Max = Right
};

enum class VerticalTextAlignment : uint32_t
{
    Top = 0,
    Center = 1,
    Bottom = 2,

    Min = Top,
    Max = Bottom
};

enum class ConfigurationChanges : uint32_t
{
    None = 0u,

    RenderLoop          = 1 << 0, // Configuration change impacts the behavior of the render loop.
    Layout              = 1 << 1, // Configuration change impacts the layout of the visualization.

    RefreshRate         = 1 << 2,
    Oscilloscope        = 1 << 3, // Configuration change impacts the oscilloscope.

    Artwork             = 1 << 4, // Configuration change impacts how the artwork is rendered.

    UserInterfaceColors = 1 << 5, // Configuration change impacts the user interface colors.

    All = ~0u,
};

inline bool operator==(ConfigurationChanges a, ConfigurationChanges b)
{
    return (size_t) a == (size_t) b;
}

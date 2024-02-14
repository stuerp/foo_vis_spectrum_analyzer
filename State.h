﻿
/** $VER: State.h (2024.02.14) P. Stuer **/

#pragma once

#include "framework.h"
#include "Support.h"

#include "WindowFunctions.h"

#include "StyleManager.h"

inline const int MinFFTSize =     2;
inline const int MaxFFTSize = 32768;

inline const double MinFFTDuration =   1.; // ms
inline const double MaxFFTDuration = 100.; // ms

inline const int MinKernelSize =  1;
inline const int MaxKernelSize = 64;

// Window Function / Brown-Puckette CQT Kernel
inline const double MinWindowParameter =  0.;
inline const double MaxWindowParameter = 10.;

inline const double MinWindowSkew = -1.;
inline const double MaxWindowSkew =  1.;

inline const double MinReactionAlignment = 0.;
inline const double MaxReactionAlignment = 5.;

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
inline const double MaxTimeResolution = 1000.;

inline const double MinSWIFTBandwidth = 0.;
inline const double MaxSWIFTBandwidth = 8.;

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
inline const double MaxAmplitude =    0.; // dB

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
inline const double MinArtworkOpacity = 0.f;
inline const double MaxArtworkOpacity = 1.f;

inline const uint32_t MinArtworkColors = 2;
inline const uint32_t MaxArtworkColors = 16;

inline const double MinLightnessThreshold = 0.f;
inline const double MaxLightnessThreshold = 1.f;

inline const uint32_t AllChannels = ((1 << audio_chunk::defined_channel_count) - 1);



inline const double MinOpacity = 0.f;
inline const double MaxOpacity = 1.f;

inline const double MinThickness =  0.f;
inline const double MaxThickness = 10.f;



enum class Transform
{
    FFT = 0,
    CQT = 1,
    SWIFT = 2,
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

    FFTCustom   = 10,
    FFTDuration = 11,
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

/// <summary>
/// Represents the configuration of the component.
/// </summary>
class State
{
public:
    State();

    State & operator=(const State & other);

    virtual ~State() { }

    void Reset() noexcept;

    void Read(stream_reader * reader, size_t size, abort_callback & abortHandler = fb2k::noAbort) noexcept;
    void Write(stream_writer * writer, abort_callback & abortHandler = fb2k::noAbort) const noexcept;

    /// <summary>
    /// Gets the duration (in ms) of the window that will be rendered.
    /// </summary>
    double GetWindowDurationInMs() const noexcept
    {
        return (double) _WindowDuration * 0.001;
    }

public:
    RECT _DialogBounds;                                                 // Will be initialized in OnInitDialog()
    size_t _PageIndex;

    size_t _RefreshRateLimit;                                           // Hz
    bool _ShowFrameCounter;
    bool _UseHardwareRendering;
    bool _UseAntialiasing;

    size_t _WindowDuration;                                             // μs

    #pragma region Transform

        Transform _Transform;                                           // FFT, CQT or SWIFT

        WindowFunctions _WindowFunction;
        double _WindowParameter;                                        // 0 .. 10, Parameter used for certain window functions like Gaussian and Kaiser windows. Defaults to 1.
        double _WindowSkew;                                             // -1 .. 1, Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples. Defaults to 0 (None).
        bool _Truncate;
        double _ReactionAlignment;                                      // Like in Vizzy.io. Controls the delay between the actual playback and the visualization.
                                                                        // < 0: All samples are ahead the actual playback (with the first sample equal to the actual playback)
                                                                        //   0: The first half of samples are behind the actual playback and the second half are ahead of it (just like original foo_musical_spectrum and basically any get_spectrum_absolute() visualizations
                                                                        // > 0: All samples are behind the playback (similar to VST audio analyzer plugins like Voxengo SPAN) with the last sample equal to the actual playback.

        uint32_t _SelectedChannels;

    #pragma endregion

    #pragma region FFT

        FFTMode _FFTMode;                                               // bins
        size_t _FFTCustom;                                              // bins, Custom FFT size
        double _FFTDuration;                                            // ms, FFT size calculated based on the sample rate

        int _KernelSize = 32;                                           // Lanczos interpolation kernel size, 1 .. 64
        SummationMethod _SummationMethod;
        bool _SmoothLowerFrequencies;                                   // When enabled, the bandpower part only gets used when number of FFT bins to sum for each band is at least two or more (rather than one in the case of this parameter is set to disabled, which of course, has artifacts on lower frequencies similar to ones seen in spectrum visualizations on YouTube videos generated by ZGameEditor Visualizer that comes with FL Studio like this), otherwise a sinc/Lanczos-interpolated spectrum.
        bool _SmoothGainTransition;                                     // Smoother frequency slope on sum modes

        Mapping _MappingMethod;                                         // Determines how the FFT coefficients are mapped to the frequency bins.

        // Brown-Puckette CQT-specific
        double _BandwidthOffset;                                        // 0.0 .. 1.0, Transition smoothness
        double _BandwidthCap;                                           // 0.0 .. 1.0, Minimum Brown-Puckette kernel size
        double _BandwidthAmount;                                        // 0 .. 256, Brown-Puckette kernel size
        bool _GranularBW;                                               // True: Don't constrain bandwidth to powers of 2.

        WindowFunctions _KernelShape;
        double _KernelShapeParameter;                                   // 0 .. 10, Used for certain window functions like Gaussian and Kaiser windows. Defaults to 1.
        double _KernelAsymmetry;                                        // -1 .. 1, Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples. Defaults to 0 (None).

    #pragma endregion

    #pragma region CQT

        double _CQTBandwidthOffset;
        double _CQTAlignment;
        double _CQTDownSample;

    #pragma endregion

    #pragma region SWIFT

        size_t _FilterBankOrder;                                        // 1 .. 8, SWIFT filter bank order
        double _TimeResolution;                                         // 0 .. 1000, Max. time resolution
        double _SWIFTBandwidth;                                         // 0 .. 8, SWIFT Bandwidth

    #pragma endregion

    #pragma region Frequencies

        FrequencyDistribution _FrequencyDistribution;

        // Frequency range
        size_t _NumBands;                                               // Number of frequency bands, 2 .. 512
        double _LoFrequency;                                            // Hz, 0 .. 96000
        double _HiFrequency;                                            // Hz, 0 .. 96000

        // Note range
        uint32_t _MinNote;                                              // Minimum note, 0 .. 143, 12 octaves
        uint32_t _MaxNote;                                              // Maximum note, 0 .. 143, 12 octaves
        uint32_t _BandsPerOctave;                                       // Bands per octave, 1 .. 48
        double _Pitch;                                                  // Hz, 0 .. 96000, Octave bands tuning (nearest note = tuning frequency in Hz)
        int _Transpose;                                                 // Transpose, -24 ..24 semitones

        ScalingFunction _ScalingFunction;

        double _SkewFactor;                                             // Affects any adjustable frequency scaling functions like hyperbolic sine and nth root. Higher values means more linear spectrum (in the case of Avee Player's frequency distribution, exactly linear when this parameter is 1), 0.0 .. 1.0
        double _Bandwidth;                                              // Distance between low and high frequency boundaries for each band, More useful for constant-Q/variable-Q transforms and Mel/triangular filterbank energies (higher values smooths out the spectrum and reduces the visual noise) than bandpower mode that we have currently at the time, 0.0 .. 64.0

    #pragma endregion

    #pragma region Filters

        WeightingType _WeightingType;

        double _SlopeFunctionOffset;                                    // 0..8, Slope function offset expressed in sample rate / FFT size in samples.

        double _Slope;                                                  // -12 .. 12, Frequency slope (dB per octave)
        double _SlopeOffset;                                            // 0 .. 96000, Frequency slope offset (Hz = 0dB)

        double _EqualizeAmount;                                         // -12 .. 12, Equalize amount
        double _EqualizeOffset;                                         // 0 .. 96000, Equalize offset
        double _EqualizeDepth;                                          // 0 .. 96000, Equalize depth

        double _WeightingAmount;                                        // -1 .. 1, Weighting amount

    #pragma endregion

    #pragma region Rendering

        #pragma region X axis

            XAxisMode _XAxisMode;
            bool _XAxisTop;
            bool _XAxisBottom;

        #pragma endregion

        #pragma region Y axis

            YAxisMode _YAxisMode;
            bool _YAxisLeft;
            bool _YAxisRight;

            double _AmplitudeLo;                                        // Lower amplitude, -120.0 .. 0.0
            double _AmplitudeHi;                                        // Upper amplitude, -120.0 .. 0.0
            double _AmplitudeStep;

            bool _UseAbsolute;                                          // Linear/n-th root scaling: Sets the min. dB range to -∞ dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
            double _Gamma;                                              // Linear/n-th root scaling: Index n of the n-th root calculation, 0.5 .. 10.0

        #pragma endregion

        #pragma region Common

            bool _ShowToolTips;                                         // True if tooltips should be displayed.

            SmoothingMethod _SmoothingMethod;
            double _SmoothingFactor;                                    // Smoothing factor, 0.0 .. 1.0

            BackgroundMode _BackgroundMode;
            FLOAT _ArtworkOpacity;                                      // 0.0 .. 1.0
            pfc::string _ArtworkFilePath;                               // Script that generates a valid file path to load artwork from.

            uint32_t _NumArtworkColors;                                 // Number of colors to select from the artwork.
            FLOAT _LightnessThreshold;                                  // 0.0 .. 1.0
            FLOAT _TransparencyThreshold;                               // 0.0 .. 1.0 (Not configurable)

            ColorOrder _ColorOrder;

        #pragma endregion

        #pragma region Visualization

            VisualizationType _VisualizationType;

            PeakMode _PeakMode;
            double _HoldTime;                                       // Peak hold time, 0.0 .. 120.0
            double _Acceleration;                                   // Peak fall acceleration rate, 0.0 .. 2.0

            #pragma region Bars

                bool _LEDMode;                                          // True if the bars will be drawn as LEDs.

            #pragma endregion

            #pragma region Curve

            #pragma endregion

        #pragma endregion

    #pragma endregion

    StyleManager _StyleManager;

    #pragma region Not Serialized

    bool _IsDUI;                                                        // True if the Default User Interface is being used.
    bool _UseToneGenerator;                                             // True if the tone generator is used instead of the visualisation stream the collect audio chunks. Mainly for testing and debugging purposes.

    std::vector<D2D1_COLOR_F> _UserInterfaceColors;

    D2D1_COLOR_F _DominantColor;                                        // The current dominant color extracted from the artwork bitmap.
    GradientStops _GradientStops;                                       // The current gradient stops.
    GradientStops _ArtworkGradientStops;                                // The current gradient stops extracted from the artwork bitmap.

    bool _NewArtworkParameters;                                         // True when the parameters to calculate the artwork palette have changed.

    int _CurrentStyle;

    #pragma endregion

public:
    double ScaleA(double value) const;

private:
    void ConvertColorSettings() noexcept;
    const GradientStops SelectGradientStops(ColorScheme colorScheme) const noexcept;

private: // Deprecated
    bool _UseZeroTrigger;                                               // Deprecated

    ColorScheme _ColorScheme;
    std::vector<D2D1_GRADIENT_STOP> _CustomGradientStops;

    D2D1::ColorF _BackColor = D2D1::ColorF(D2D1::ColorF::Black);
    bool _UseCustomBackColor;

    D2D1::ColorF _XTextColor = D2D1::ColorF(D2D1::ColorF::White);
    bool _UseCustomXTextColor;

    D2D1::ColorF _XLineColor = D2D1::ColorF(D2D1::ColorF::White);
    bool _UseCustomXLineColor;

    D2D1::ColorF _YTextColor = D2D1::ColorF(D2D1::ColorF::White);
    bool _UseCustomYTextColor;

    D2D1::ColorF _YLineColor = D2D1::ColorF(D2D1::ColorF::White);
    bool _UseCustomYLineColor;

    D2D1::ColorF _LightBandColor = D2D1::ColorF(.2f, .2f, .2f, .7f);
    D2D1::ColorF _DarkBandColor = D2D1::ColorF(.2f, .2f, .2f, .7f);

    FLOAT _LineWidth;
    D2D1::ColorF _LineColor = _DefLineColor;
    bool _UseCustomLineColor;
    D2D1::ColorF _PeakLineColor = _DefPeakLineColor;
    bool _UseCustomPeakLineColor;
    FLOAT _AreaOpacity;                                     // 0.0 .. 1.0

    const D2D1::ColorF _DefLineColor = D2D1::ColorF(0.f, 0.f, 0.f, 0.f);
    const D2D1::ColorF _DefPeakLineColor = D2D1::ColorF(0.f, 0.f, 0.f, 0.f);

    bool _DrawBandBackground;                               // True if the background for each band should be drawn.
    bool _HorizontalGradient;                               // True if the gradient will be used to paint horizontally.

private:
    const size_t _CurrentVersion = 16;
};

/** $VER: Configuration.h (2023.12.03) P. Stuer **/

#pragma once

#include "framework.h"

#include "Math.h"
#include "WindowFunctions.h"

inline const double MinWindowSkew = -1.;
inline const double MaxWindowSkew =  1.;

inline const double MinWindowParameter =  0.;
inline const double MaxWindowParameter = 10.;

inline const int MinFFTSize =     1;
inline const int MaxFFTSize = 32768;

inline const double MinFFTDuration =   1.; // ms
inline const double MaxFFTDuration = 100.; // ms

inline const int MinKernelSize =  1;
inline const int MaxKernelSize = 64;

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

inline const double MinSmoothingFactor = 0.;
inline const double MaxSmoothingFactor = 1.;

inline const double MinHoldTime =   0.;
inline const double MaxHoldTime = 120.;

inline const double MinAcceleration = 0.;
inline const double MaxAcceleration = 2.;

inline const uint32_t AllChannels = ((1 << audio_chunk::defined_channel_count) - 1);

enum class Transform
{
    FFT = 0,
    CQT = 1
};

enum class FFTSize
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
};

enum class FrequencyDistribution
{
    Linear = 0,
    Octaves = 1,
    AveePlayer = 2,
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
    Average = 0,
    Peak = 1
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
    Logarithmic = 2,
};

enum class ColorScheme
{
    Solid = 0,
    Custom = 1,

    Prism1 = 2,
    Prism2 = 3,
    Prism3 = 4,

    foobar2000 = 5,
    foobar2000DarkMode = 6,

    Fire = 7,
};

enum class PeakMode
{
    None = 0,

    Classic = 1,
    Gravity = 2,
    AIMP = 3,
    FadeOut = 4,
};

enum class LogLevel
{
    Trace = 0,          // Logs that contain the most detailed messages. These messages may contain sensitive application data. These messages are disabled by default and should never be enabled in a production environment.
    Debug = 1,          // Logs that are used for interactive investigation during development. These logs should primarily contain information useful for debugging and have no long-term value.
    Information = 2,    // Logs that track the general flow of the application. These logs should have long-term value.
    Warning = 3,        // Logs that highlight an abnormal or unexpected event in the application flow, but do not otherwise cause the application execution to stop.
    Error = 4,          // Logs that highlight when the current flow of execution is stopped due to a failure. These should indicate a failure in the current activity, not an application-wide failure.
    Critical = 5,       // Logs that describe an unrecoverable application or system crash, or a catastrophic failure that requires immediate attention.

    None = 6,           // Not used for writing log messages. Specifies that a logging category should not write any messages.
};

/// <summary>
/// Represents the configuration of the spectrum analyzer.
/// </summary>
class Configuration
{
public:
    Configuration();

    Configuration & operator=(const Configuration & other);

    virtual ~Configuration() { }

    void Reset() noexcept;

    void Read(ui_element_config_parser & parser);
    void Write(ui_element_config_builder & builder) const;

    void Read();

    /// <summary>
    /// Gets the duration (in ms) of the window that will be rendered.
    /// </summary>
    double GetWindowDurationInMs() const noexcept
    {
        return (double) _WindowDuration * 0.001;
    }

public:
    RECT _DialogBounds;

    bool _UseHardwareRendering;
    bool _UseZeroTrigger;
    bool _UseAntialiasing;

    size_t _WindowDuration;
    size_t _RefreshRateLimit;                                           // Hz

    #pragma region Transform
        Transform _Transform;                                           // FFT or CQT

        WindowFunctions _WindowFunction;
        double _WindowParameter;                                        // 0 .. 10
        double _WindowSkew;                                             // -1 .. 1
        bool _Truncate;

        uint32_t _SelectedChannels;
    #pragma endregion

    #pragma region FFT
        FFTSize _FFTSize;
        size_t _FFTCustom;                                              // samples, Custom FFT size
        double _FFTDuration;                                            // ms, FFT size calculated based on the sample rate

        int _KernelSize = 32;                                           // Lanczos interpolation kernel size, 1 .. 64
        SummationMethod _SummationMethod = SummationMethod::Maximum;
        bool _SmoothLowerFrequencies;                                   // When enabled, the bandpower part only gets used when number of FFT bins to sum for each band is at least two or more (rather than one in the case of this parameter is set to disabled, which of course, has artifacts on lower frequencies similar to ones seen in spectrum visualizations on YouTube videos generated by ZGameEditor Visualizer that comes with FL Studio like this), otherwise a sinc/Lanczos-interpolated spectrum.
        bool _SmoothGainTransition;                                     // Smoother frequency slope on sum modes

        Mapping _MappingMethod;                                         // Determines how the FFT coefficients are mapped to the frequency bins.
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

        ScalingFunction _ScalingFunction = ScalingFunction::Logarithmic;

        double _SkewFactor;                                             // Affects any adjustable frequency scaling functions like hyperbolic sine and nth root. Higher values means more linear spectrum (in the case of Avee Player's frequency distribution, exactly linear when this parameter is 1), 0.0 .. 1.0
        double _Bandwidth;                                              // Distance between low and high frequency boundaries for each band, More useful for constant-Q/variable-Q transforms and Mel/triangular filterbank energies (higher values smooths out the spectrum and reduces the visual noise) than bandpower mode that we have currently at the time, 0.0 .. 64.0
    #pragma endregion

    #pragma region Rendering
        D2D1::ColorF _BackColor = D2D1::ColorF(D2D1::ColorF::Black);    // Background color of the element

        #pragma region X axis
            XAxisMode _XAxisMode;

            D2D1::ColorF _XTextColor = D2D1::ColorF(D2D1::ColorF::White);
            D2D1::ColorF _XLineColor = D2D1::ColorF(D2D1::ColorF::White);
        #pragma endregion

        #pragma region Y axis
            YAxisMode _YAxisMode;

            D2D1::ColorF _YTextColor = D2D1::ColorF(D2D1::ColorF::White);
            D2D1::ColorF _YLineColor = D2D1::ColorF(D2D1::ColorF::White);

            double _AmplitudeLo;                                         // Lower amplitude, -120.0 .. 0.0
            double _AmplitudeHi;                                         // Upper amplitude, -120.0 .. 0.0
            double _AmplitudeStep;

            bool _UseAbsolute = true;                                   // Logarithmic scale: Sets the min. dB range to -Infinity dB (0.0 on linear amplitude) when enabled. This only applies when not using logarithmic amplitude scale (or in other words, using linear/nth root amplitude scaling) as by mathematical definition. Logarithm of any base of zero is always -Infinity.
            double _Gamma;                                              // Logarithmic scale: Gamma, 0.5 .. 10.0
        #pragma endregion

        #pragma region Bands
            ColorScheme _ColorScheme;
            std::vector<D2D1_GRADIENT_STOP> _GradientStops;             // The current gradient stops. Will not be persisted.
            std::vector<D2D1_GRADIENT_STOP> _CustomGradientStops;       // The custom gradient stops.

            bool _DrawBandBackground;                                   // True if the background for each band should be drawn.
            D2D1::ColorF _BandBackColor = D2D1::ColorF(.2f, .2f, .2f, .7f);
            bool _ShowToolTips;

            SmoothingMethod _SmoothingMethod = SmoothingMethod::Average;
            double _SmoothingFactor;                                    // Smoothing factor, 0.0 .. 1.0

            PeakMode _PeakMode;
            double _HoldTime;                                           // Peak hold time, 0.0 .. 120.0
            double _Acceleration;                                       // Peak fall acceleration rate, 0.0 .. 2.0
        #pragma endregion
    #pragma endregion

    #pragma region Colors

    #pragma endregion

    LogLevel _LogLevel;
/*
    bandwidthOffset: 1,

    timeAlignment: 1,
    downsample: 0,
    clampPeaks: true,

    labelTuning: 440,

    showDC: true,
    showNyquist: true,

    diffLabels: false,
    compensateDelay: false
*/
public:
    /// <summary>
    /// Scales the specified value to a relative amplitude between 0.0 and 1.0.
    /// </summary>
    /// <remarks>FIXME: This should not live here but it's pretty convenient...</remarks>
    double ScaleA(double value) const
    {
        if ((_YAxisMode == YAxisMode::Decibels) || (_YAxisMode == YAxisMode::None))
            return Map(ToDecibel(value), _AmplitudeLo, _AmplitudeHi, 0.0, 1.0);

        double Exponent = 1.0 / _Gamma;

        return Map(::pow(value, Exponent), _UseAbsolute ? 0.0 : ::pow(ToMagnitude(_AmplitudeLo), Exponent), ::pow(ToMagnitude(_AmplitudeHi), Exponent), 0.0, 1.0);
    }

    void UpdateGradient();

private:
    const size_t _CurrentVersion = 7;
};

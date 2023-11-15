
/** $VER: Configuration.h (2023.11.14) P. Stuer **/

#pragma once

#include "framework.h"

#include "FFT.h"

enum class FrequencyDistributions
{
    Frequencies = 0,
    Octaves = 1,
    AveePlayer = 2,
};

enum class ScalingFunctions
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
    Bands = 0,
    Decades = 1,
    Octaves = 2,
    Notes = 3,
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

    Configuration(const Configuration &) = delete;
    Configuration & operator=(const Configuration &) = delete;
    Configuration(Configuration &&) = delete;
    Configuration & operator=(Configuration &&) = delete;

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
    RECT _OptionsRect;

    bool _UseHardwareRendering;
    bool _UseZeroTrigger;
    bool _UseAntialiasing;

    size_t _WindowDuration;
    size_t _RefreshRateLimit;   // in Hz

    FFTSize _FFTSize;           // Power of 2
    FrequencyDistributions FrequencyDistribution;

    // Common
    size_t NumBands =    320;  // Number of frequency bands, 2 .. 512
    uint32_t MinFrequency =    20;  // Hz, 0 .. 96000
    uint32_t MaxFrequency = 20000;  // Hz, 0 .. 96000

    // Octaves
    uint32_t BandsPerOctave =  12;    // Bands per octave, 1 .. 48
    uint32_t MinNote =   0;    // Minimum note, 0 .. 143, 12 octaves
    uint32_t MaxNote = 143;    // Maximum note, 0 .. 143, 12 octaves
    int Detune       =   0;    // Detune, -24 ..24
    double _Pitch     = 440.0;  // Hz, 0 .. 96000, Octave bands tuning (nearest note = tuning frequency in Hz)

    // Frequencies
    ScalingFunctions ScalingFunction = ScalingFunctions::Logarithmic;

    double SkewFactor = 0.0;   // Hz linear factor, 0.0 .. 1.0
    double Bandwidth = 0.5;        // Bandwidth, 0.0 .. 64.0

    SmoothingMethod _SmoothingMethod = SmoothingMethod::Average;    // Smoothing method
    double SmoothingFactor = 0.0;                                            // Smoothing constant, 0.0 .. 1.0

    int interpSize = 32;                                    // Lanczos interpolation kernel size, 1 .. 64
    SummationMethod _SummationMethod = SummationMethod::Maximum;  // Band power summation method
    bool smoothInterp = true;                               // Smoother bin interpolation on lower frequencies
    bool smoothSlope = true;                                // Smoother frequency slope on sum modes

    // ascale() Amplitude Scale
    bool UseDecibels = true;                               // Use decibel scale or logaritmic amplitude

    double MinDecibels = -90.;                             // Lower amplitude, -120.0 .. 0.0
    double MaxDecibels =   0.;                             // Upper amplitude, -120.0 .. 0.0

    bool _UseAbsolute = true;                               // Use absolute value

    double Gamma = 1.;                                     // Gamma, 0.5 .. 10

    XAxisMode _XAxisMode;

    ColorScheme _ColorScheme;

    LogLevel _LogLevel;
/*
    type: 'fft',
    bandwidthOffset: 1,

    windowFunction: 'hann',
    windowParameter: 1,
    windowSkew: 0,

    timeAlignment: 1,
    downsample: 0,
    useComplex: true,
    holdTime: 30,
    fallRate: 0.5,
    clampPeaks: true,
    peakMode: 'gravity',
    showPeaks: true,

    freeze: false,
    color: 'none',
    showLabels: true,
    showLabelsY: true,
    labelTuning: 440,
    showDC: true,
    showNyquist: true,
    mirrorLabels: true,
    diffLabels: false,
    darkMode: false,
    compensateDelay: false
*/

private:
    const size_t _Version = 2;
};

extern Configuration _Configuration;

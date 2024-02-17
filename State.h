﻿
/** $VER: State.h (2024.02.16) P. Stuer **/

#pragma once

#include "framework.h"
#include "Support.h"

#include "Constants.h"
#include "WindowFunctions.h"

#include "StyleManager.h"

#include <vector>

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
        bool _UseGranularBandwidth;                                               // True: Don't constrain bandwidth to powers of 2.

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
        size_t _BandCount;                                               // Number of frequency bands, 2 .. 512
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
            bool _SuppressMirrorImage;                                  // True if the mirror image of the spectrum is not rendered.

            SmoothingMethod _SmoothingMethod;
            double _SmoothingFactor;                                    // Smoothing factor, 0.0 .. 1.0

            BackgroundMode _BackgroundMode;
            FLOAT _ArtworkOpacity;                                      // 0.0 .. 1.0
            pfc::string _ArtworkFilePath;                               // Script that generates a valid file path to load artwork from.
            FitMode _FitMode;

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

    size_t _BinCount;

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
    const size_t _CurrentVersion = 17;
};

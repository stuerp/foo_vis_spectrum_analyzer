
/** $VER: State.cpp (2026.03.21) P. Stuer **/

#include "pch.h"
#include "State.h"

#include "Gradients.h"
#include "Resources.h"
#include "Log.h"

#include <sdk\file.h>
#include <sdk\filesystem.h>

#include <pfc\string-conv-lite.h>

using namespace pfc;
using namespace stringcvt;

#pragma warning(push)
#pragma warning(disable: 4868) // compiler may not enforce left-to-right evaluation order in braced initializer list

#include <nlohmann\json.hpp>

using json = nlohmann::ordered_json;

#pragma warning(pop)

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
state_t::state_t() noexcept
{
    Reset();
}

/// <summary>
/// Resets this instance.
/// </summary>
void state_t::Reset() noexcept
{
    _Bounds = { };
    _PageIndex = 0;

    _RefreshRateLimit = 20; // Hz
    _SleepTime = 200; // μs

    _UseHardwareRendering = true;
    _UseAntialiasing = true;

    _UseZeroTrigger_Deprecated = false;
    _WindowDuration = 50;

    // Transform
    _TransformMethod = TransformMethod::FFT;

    _WindowFunction = WindowFunction::Hann;
    _WindowParameter = 1.;
    _WindowSkew = 0.;
    _Truncate = true;

    _ReactionAlignment = 0.25;

    _Channels_Deprecated = AllChannels;

    // FFT
    _FFTMode = FFTMode::FFT4096;
    _FFTCustom = 4096;
    _FFTDuration = 100.;

    _MappingMethod = Mapping::Standard;

    // CQT
    _CQTBandwidthOffset = 1.;
    _CQTAlignment = 1.;
    _CQTDownSample = 0.;

    // Brown-Puckette CQT-specific
    _BandwidthOffset = 1.;
    _BandwidthCap = 1.;
    _BandwidthAmount = 6.;
    _UseGranularBandwidth = true;

    _KernelShape = WindowFunction::Nuttall;
    _KernelShapeParameter = 1.;
    _KernelAsymmetry = 0.;

    // IIR
    _FilterBankOrder = 4;
    _TimeResolution = 600.;
    _IIRBandwidth = 1.;
    _ConstantQ = true;
    _CompensateBandwidth = true;
    _UsePreWarpedQ = false;

    // Frequencies
    _FrequencyDistribution = FrequencyDistribution::Octaves;

    // Frequency range
    _BandCount = 320;
    _LoFrequency = 20.;
    _HiFrequency = 20000.;

    // Note range
    _LoNote = 0;
    _HiNote = 126;
    _BandsPerOctave = 12;
    _TuningPitch = 440.0;
    _Transpose = 0;

    _ScalingFunction = ScalingFunction::Logarithmic;
    _SkewFactor = 0.0;
    _Bandwidth = 0.5;

    // Filters
    _WeightingType = WeightingType::None;

    _SlopeFunctionOffset = 1.;

    _Slope = 0.;
    _SlopeOffset = 1000.;

    _EqualizeAmount = 0.;
    _EqualizeOffset = 44100.;
    _EqualizeDepth = 1024.;

    _WeightingAmount = 0.;

    _SmoothingMethod = SmoothingMethod::Average;
    _SmoothingFactor = 0.5;

    _KernelSize = 32;
    _AggregationMethod = AggregationMethod::Maximum;
    _SmoothLowerFrequencies = true;
    _SmoothGainTransition = true;

    // Rendering parameters
    _BackColor_Deprecated = D2D1::ColorF(D2D1::ColorF::Black);                  // Deprecated
    _UseCustomBackColor_Deprecated = true;                                      // Deprecated

    // X axis
    _XAxisMode_Deprecated = XAxisMode::Notes;
    _XAxisTop_Deprecated = true;
    _XAxisBottom_Deprecated = true;

    _XTextColor_Deprecated = D2D1::ColorF(D2D1::ColorF::White);                 // Deprecated
    _UseCustomXTextColor_Deprecated = true;                                     // Deprecated

    _XLineColor_Deprecated = D2D1::ColorF(D2D1::ColorF::White);                 // Deprecated
    _UseCustomXLineColor_Deprecated = true;                                     // Deprecated

    // Y axis
    _YAxisMode_Deprecated = YAxisMode::Decibels;
    _YAxisLeft_Deprecated = true;
    _YAxisRight_Deprecated = true;

    _YTextColor_Deprecated = D2D1::ColorF(D2D1::ColorF::White);                 // Deprecated
    _UseCustomYTextColor_Deprecated = true;                                     // Deprecated
 
    _YLineColor_Deprecated = D2D1::ColorF(.25f, .25f, .25f, 1.f);               // Deprecated
    _UseCustomYLineColor_Deprecated = true;                                     // Deprecated

    _AmplitudeLo_Deprecated = -90.;
    _AmplitudeHi_Deprecated =   0.;
    _AmplitudeStep_Deprecated = -6.;

    _UseAbsolute_Deprecated = true;
    _Gamma_Deprecated = 1.;

    // Common
    _ColorScheme_Deprecated = ColorScheme::Prism1;

//  _GradientStops = GetBuiltInGradientStops(_ColorScheme_Deprecated);
    _CustomGradientStops_Deprecated = GetBuiltInGradientStops(ColorScheme::Custom);

    _ShowToolTipsAlways = true;
    _SuppressMirrorImage = true;
    _VisualizeDuringPause = true;

    // Artwork
    _NumArtworkColors = 10;
    _LightnessThreshold = 250.f / 255.f;
    _TransparencyThreshold = 125.f / 255.f;

    _ColorOrder = ColorOrder::None;
    _ArtworkType = ArtworkType::Front;

    _BackgroundMode_Deprecated = BackgroundMode::Artwork;
    _ShowArtworkOnBackground = true;
    _ArtworkOpacity = 1.f;
    _ArtworkFilePath.clear();
    _FitMode = FitMode::FitBig;
    _FitWindow = false;

    /** Graphs **/

    _GraphDescriptions.clear();

    _GraphDescriptions.push_back(graph_description_t(L"Stereo"));

    _VerticalLayout = false;

    _GridRowCount = 1;
    _GridColumnCount = 1;

    /** Visualization **/

    _VisualizationType = VisualizationType::Bars;

    // Bars
    _DrawBandBackground_Deprecated = true;
    _LightBandColor_Deprecated = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _DarkBandColor_Deprecated = D2D1::ColorF(.2f, .2f, .2f, .7f);

    _LEDMode = false;
    _LEDLight = 2.f;
    _LEDGap = 2.f;
    _LEDIntegralSize = false;

    _HorizontalGradient_Deprecated = false;

    _PeakMode = PeakMode::Classic;
    _HoldTime = 30.;
    _Acceleration = 0.5;

    // Radial Bars
    _InnerRadius = 0.2f;
    _OuterRadius = 1.0f;
    _AngularVelocity = 60.f; // degrees / sec

    // Curve
    _LineWidth_Deprecated = 2.f;
    _LineColor_Deprecated = D2D1::ColorF(D2D1::ColorF::White);
    _UseCustomLineColor_Deprecated = false;
    _PeakLineColor_Deprecated = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _UseCustomPeakLineColor_Deprecated = false;
    _AreaOpacity_Deprecated = 0.5f;

    // Spectrogram
    _IsScrollingSpectrogram = true;
    _IsHorizontalSpectrogram = true;
    _UseSpectrumBarMetrics = false;

    // Peak Meter
    _IsHorizontalPeakMeter = false;
    _HasRMSPlus3 = false;
    _RMSWindow = .300; // seconds
    _BarGap = 1.f; // pixels
    _HasCenterScale = false;
    _HasScaleLines = true;
    _MaxBarSize = 0.f; // pixels

    // Level Meter
    _ChannelPair = ChannelPair::FrontLeftRight;
    _IsHorizontalLevelMeter = false;

    // Oscilloscope
    _XYMode = false;
    _XGain = 1.f;
    _YGain = 1.f;
    _Rotation = 0.f;
    _HasPhosphorDecay = true;
    _BlurSigma = 3.f;
    _DecayFactor = 0.92f;

    // Bit Meter
    _OpacityMode = false;

    _StyleManager.Reset();

    pfc::string Path = core_api::get_profile_path();

    Path = foobar2000_io::filesystem::g_get_native_path(Path);

    _PresetsDirectoryPath = pfc::wideFromUTF8(Path);

    /** Not serialized **/

    _SampleRate = 0;

    _ActivePresetName.clear();
}

/// <summary>
/// Implements the = operator.
/// </summary>
state_t & state_t::operator=(const state_t & other) noexcept
{
    _Bounds = other._Bounds;
    _PageIndex = other._PageIndex;

    _RefreshRateLimit = other._RefreshRateLimit;

    _UseHardwareRendering = other._UseHardwareRendering;
    _UseAntialiasing = other._UseAntialiasing;

//  _UseZeroTrigger_Deprecated = other._UseZeroTrigger_Deprecated;
    _WindowDuration = other._WindowDuration;

    #pragma region Transform

        _TransformMethod = other._TransformMethod;

        _WindowFunction = other._WindowFunction;
        _WindowParameter = other._WindowParameter;
        _WindowSkew = other._WindowSkew;
        _Truncate = other._Truncate;

        _ReactionAlignment = other._ReactionAlignment;

//      _Channels_Deprecated = other._Channels_Deprecated;

    #pragma endregion

    #pragma region FFT

        _FFTMode = other._FFTMode;
        _FFTCustom = other._FFTCustom;
        _FFTDuration = other._FFTDuration;

        _KernelSize = other._KernelSize;
        _AggregationMethod = other._AggregationMethod;
        _SmoothLowerFrequencies = other._SmoothLowerFrequencies;
        _SmoothGainTransition = other._SmoothGainTransition;

        _MappingMethod = other._MappingMethod;

        #pragma region Brown-Puckette CQT

            _BandwidthOffset = other._BandwidthOffset;
            _BandwidthCap = other._BandwidthCap;
            _BandwidthAmount = other._BandwidthAmount;
            _UseGranularBandwidth = other._UseGranularBandwidth;

            _KernelShape = other._KernelShape;
            _KernelShapeParameter = other._KernelShapeParameter;
            _KernelAsymmetry = other._KernelAsymmetry;

        #pragma endregion

    #pragma endregion

    #pragma region CQT

        _CQTBandwidthOffset = other._CQTBandwidthOffset;
        _CQTAlignment = other._CQTAlignment;
        _CQTDownSample = other._CQTDownSample;

    #pragma endregion

    #pragma region IIR

        _FilterBankOrder = other._FilterBankOrder;
        _TimeResolution = other._TimeResolution;
        _IIRBandwidth = other._IIRBandwidth;
        _ConstantQ = other._ConstantQ;
        _CompensateBandwidth = other._CompensateBandwidth;
        _UsePreWarpedQ = other._UsePreWarpedQ;

    #pragma endregion

    #pragma region Frequencies

        _FrequencyDistribution = other._FrequencyDistribution;

        _BandCount = other._BandCount;
        _LoFrequency = other._LoFrequency;
        _HiFrequency = other._HiFrequency;

        // Note range
        _LoNote = other._LoNote;
        _HiNote = other._HiNote;
        _BandsPerOctave = other._BandsPerOctave;
        _TuningPitch = other._TuningPitch;
        _Transpose = other._Transpose;

        _ScalingFunction = other._ScalingFunction;
        _SkewFactor = other._SkewFactor;
        _Bandwidth = other._Bandwidth;

    #pragma endregion

    #pragma region Filters

        _WeightingType = other._WeightingType;

        _SlopeFunctionOffset = other._SlopeFunctionOffset;

        _Slope = other._Slope;
        _SlopeOffset = other._SlopeOffset;

        _EqualizeAmount = other._EqualizeAmount;
        _EqualizeOffset = other._EqualizeOffset;
        _EqualizeDepth = other._EqualizeDepth;

        _WeightingAmount = other._WeightingAmount;

    #pragma endregion

    #pragma region Rendering

//      _BackColor_Deprecated = other._BackColor_Deprecated;
//      _UseCustomBackColor_Deprecated = other._UseCustomBackColor_Deprecated;

        // X axis
//      _XAxisMode_Deprecated = other._XAxisMode_Deprecated;
//      _XAxisTop_Deprecated = other._XAxisTop_Deprecated;
//      _XAxisBottom_Deprecated = other._XAxisBottom_Deprecated;

//      _XTextColor_Deprecated = other._XTextColor_Deprecated;
//      _UseCustomXTextColor_Deprecated = other._UseCustomXTextColor_Deprecated;

//      _XLineColor_Deprecated = other._XLineColor_Deprecated;
//      _UseCustomXLineColor_Deprecated = other._UseCustomXLineColor_Deprecated;

        // Y axis
//      _YAxisMode_Deprecated = other._YAxisMode_Deprecated;
//      _YAxisLeft_Deprecated = other._YAxisLeft_Deprecated;
//      _YAxisRight_Deprecated = other._YAxisRight_Deprecated;

//      _YTextColor_Deprecated = other._YTextColor_Deprecated;
//      _UseCustomYTextColor_Deprecated = other._UseCustomYTextColor_Deprecated;

//      _YLineColor_Deprecated = other._YLineColor_Deprecated;
//      _UseCustomYLineColor_Deprecated = other._UseCustomYLineColor_Deprecated;

//      _AmplitudeLo_Deprecated = other._AmplitudeLo_Deprecated;
//      _AmplitudeHi_Deprecated = other._AmplitudeHi_Deprecated;
//      _AmplitudeStep_Deprecated = other._AmplitudeStep_Deprecated;

//      _UseAbsolute_Deprecated = other._UseAbsolute_Deprecated;

//      _Gamma_Deprecated = other._Gamma_Deprecated;

    #pragma endregion

    #pragma region Graph Common

        // Common
//      _ColorScheme_Deprecated = other._ColorScheme_Deprecated;

        _SmoothingMethod = other._SmoothingMethod;
        _SmoothingFactor = other._SmoothingFactor;

//      _GradientStops = other._GradientStops;                  // Deprecated
//      _CustomGradientStops_Deprecated = other._CustomGradientStops_Deprecated;

        _ShowToolTipsAlways = other._ShowToolTipsAlways;
        _SuppressMirrorImage = other._SuppressMirrorImage;
        _VisualizeDuringPause = other._VisualizeDuringPause;

        // Artwork
        _NumArtworkColors = other._NumArtworkColors;
        _LightnessThreshold = other._LightnessThreshold;
        _TransparencyThreshold = other._TransparencyThreshold;

        _ColorOrder = other._ColorOrder;

//      _BackgroundMode_Deprecated = other._BackgroundMode_Deprecated;

        _ShowArtworkOnBackground = other._ShowArtworkOnBackground;
        _ArtworkType = other._ArtworkType;

        _ArtworkOpacity = other._ArtworkOpacity;
        _ArtworkFilePath = other._ArtworkFilePath;
        _FitMode = other._FitMode;
        _FitWindow = other._FitWindow;

    #pragma endregion

    #pragma region Graphs

    _GraphDescriptions = other._GraphDescriptions;

    _VerticalLayout = other._VerticalLayout;

    _GridRowCount = other._GridRowCount;
    _GridColumnCount = other._GridColumnCount;

    #pragma endregion

    #pragma region Visualization

    _VisualizationType = other._VisualizationType;

    // Bars
//  _DrawBandBackground_Deprecated = other._DrawBandBackground_Deprecated;
//  _LightBandColor_Deprecated = other._LightBandColor_Deprecated;
//  _DarkBandColor_Deprecated = other._DarkBandColor_Deprecated;

    _LEDMode = other._LEDMode;
    _LEDLight = other._LEDLight;
    _LEDGap = other._LEDGap;
    _LEDIntegralSize = other._LEDIntegralSize;

//  _HorizontalGradient_Deprecated = other._HorizontalGradient_Deprecated;

    _PeakMode = other._PeakMode;
    _HoldTime = other._HoldTime;
    _Acceleration = other._Acceleration;

    // Radial Bars
    _InnerRadius = other._InnerRadius;
    _OuterRadius = other._OuterRadius;
    _AngularVelocity = other._AngularVelocity;

    // Curve
//  _LineWidth_Deprecated = other._LineWidth_Deprecated;
//  _LineColor_Deprecated = other._LineColor_Deprecated;
//  _UseCustomLineColor_Deprecated = other._UseCustomLineColor_Deprecated;
//  _PeakLineColor_Deprecated = other._PeakLineColor_Deprecated;
//  _UseCustomPeakLineColor_Deprecated = other._UseCustomPeakLineColor_Deprecated;
//  _AreaOpacity_Deprecated = other._AreaOpacity_Deprecated;

    // Spectrogram
    _IsScrollingSpectrogram = other._IsScrollingSpectrogram;
    _IsHorizontalSpectrogram = other._IsHorizontalSpectrogram;
    _UseSpectrumBarMetrics = other._UseSpectrumBarMetrics;

    // Peak Meter
    _IsHorizontalPeakMeter = other._IsHorizontalPeakMeter;
    _HasRMSPlus3 = other._HasRMSPlus3;
    _RMSWindow = other._RMSWindow;
    _BarGap = other._BarGap;
    _HasCenterScale = other._HasCenterScale;
    _HasScaleLines = other._HasScaleLines;
    _MaxBarSize = other._MaxBarSize;

    // Level Meter
    _ChannelPair = other._ChannelPair;
    _IsHorizontalLevelMeter = other._IsHorizontalLevelMeter;

    // Oscilloscope
    _XYMode = other._XYMode;
    _XGain = other._XGain;
    _YGain = other._YGain;
    _Rotation = other._Rotation;
    _HasPhosphorDecay = other._HasPhosphorDecay;
    _BlurSigma = other._BlurSigma;
    _DecayFactor = other._DecayFactor;

    // Bit Meter
    _OpacityMode = other._OpacityMode;

    #pragma endregion

    #pragma region Styles

    _StyleManager = other._StyleManager;

    #pragma endregion

    #pragma region Presets

    _PresetsDirectoryPath = other._PresetsDirectoryPath;

    #pragma endregion

    #pragma region Not serialized

    _BinCount = other._BinCount;
    _ActivePresetName = other._ActivePresetName;

    #pragma endregion

    return *this;
}

/// <summary>
/// Reads this instance with the specified reader.
/// </summary>
void state_t::Read(stream_reader * stream, size_t size, abort_callback & abortHandler, bool isPreset) noexcept
{
    size_t Version;

    if (size < sizeof(Version))
        return;

    try
    {
        stream->read(&Version, sizeof(Version), abortHandler);

        if (Version > 9999) // Just a version number that seems sane...
            return;

        stream->read(&_Bounds, sizeof(_Bounds), abortHandler);

        stream->read(&_RefreshRateLimit, sizeof(_RefreshRateLimit), abortHandler);

        stream->read(&_UseHardwareRendering, sizeof(_UseHardwareRendering), abortHandler);
        stream->read(&_UseAntialiasing, sizeof(_UseAntialiasing), abortHandler);

        stream->read(&_UseZeroTrigger_Deprecated, sizeof(_UseZeroTrigger_Deprecated), abortHandler);
        stream->read(&_WindowDuration, sizeof(_WindowDuration), abortHandler); _WindowDuration = std::clamp<size_t>(_WindowDuration, 50, 800);

        stream->read(&_TransformMethod, sizeof(_TransformMethod), abortHandler);

    #pragma region FFT

        stream->read(&_FFTMode, sizeof(_FFTMode), abortHandler);

        // v17: FFTMode::65536 was added before FFTMode::Custom
        if (Version < 17)
            _FFTMode = (FFTMode) ((int) _FFTMode + 1);

        stream->read(&_FFTCustom, sizeof(_FFTCustom), abortHandler);
        stream->read(&_FFTDuration, sizeof(_FFTDuration), abortHandler);

        stream->read(&_MappingMethod, sizeof(_MappingMethod), abortHandler);
        stream->read(&_SmoothingMethod, sizeof(_SmoothingMethod), abortHandler);

        // v15: SmoothingMethod::None was inserted before SmoothingMethod::Average.
        if (Version < 15)
            _SmoothingMethod = (SmoothingMethod) ((int) _SmoothingMethod + 1);

        stream->read(&_SmoothingFactor, sizeof(_SmoothingFactor), abortHandler);
        stream->read(&_KernelSize, sizeof(_KernelSize), abortHandler);
        stream->read(&_AggregationMethod, sizeof(_AggregationMethod), abortHandler);
        stream->read(&_SmoothLowerFrequencies, sizeof(_SmoothLowerFrequencies), abortHandler);
        stream->read(&_SmoothGainTransition, sizeof(_SmoothGainTransition), abortHandler);

    #pragma endregion

    #pragma region Frequencies

        stream->read(&_FrequencyDistribution, sizeof(_FrequencyDistribution), abortHandler);

        stream->read(&_BandCount, sizeof(_BandCount), abortHandler);

        stream->read(&_LoFrequency, sizeof(_LoFrequency), abortHandler);
        stream->read(&_HiFrequency, sizeof(_HiFrequency), abortHandler);

        stream->read(&_LoNote, sizeof(_LoNote), abortHandler);
        stream->read(&_HiNote, sizeof(_HiNote), abortHandler);
        stream->read(&_BandsPerOctave, sizeof(_BandsPerOctave), abortHandler);
        stream->read(&_TuningPitch, sizeof(_TuningPitch), abortHandler);
        stream->read(&_Transpose, sizeof(_Transpose), abortHandler);

        stream->read(&_ScalingFunction, sizeof(_ScalingFunction), abortHandler);
        stream->read(&_SkewFactor, sizeof(_SkewFactor), abortHandler);
        stream->read(&_Bandwidth, sizeof(_Bandwidth), abortHandler);

    #pragma endregion

    #pragma region Rendering

        stream->read(&_BackColor_Deprecated, sizeof(_BackColor_Deprecated), abortHandler);

        stream->read(&_XAxisMode_Deprecated, sizeof(_XAxisMode_Deprecated), abortHandler);

        stream->read(&_YAxisMode_Deprecated, sizeof(_YAxisMode_Deprecated), abortHandler);

        stream->read(&_AmplitudeLo_Deprecated, sizeof(_AmplitudeLo_Deprecated), abortHandler);
        stream->read(&_AmplitudeHi_Deprecated, sizeof(_AmplitudeHi_Deprecated), abortHandler);
        stream->read(&_UseAbsolute_Deprecated, sizeof(_UseAbsolute_Deprecated), abortHandler);
        stream->read(&_Gamma_Deprecated, sizeof(_Gamma_Deprecated), abortHandler);

        stream->read(&_ColorScheme_Deprecated, sizeof(_ColorScheme_Deprecated), abortHandler);

        if ((Version <= 9) && (_ColorScheme_Deprecated != ColorScheme::Solid) && (_ColorScheme_Deprecated != ColorScheme::Custom))
            _ColorScheme_Deprecated = (ColorScheme) ((int) _ColorScheme_Deprecated + 1); // ColorScheme::Artwork was added after ColorScheme::Custom

        stream->read(&_DrawBandBackground_Deprecated, sizeof(_DrawBandBackground_Deprecated), abortHandler);

        stream->read(&_PeakMode, sizeof(_PeakMode), abortHandler);
        stream->read(&_HoldTime, sizeof(_HoldTime), abortHandler);
        stream->read(&_Acceleration, sizeof(_Acceleration), abortHandler);

    #pragma endregion

        _CustomGradientStops_Deprecated.clear();

        size_t Count; stream->read(&Count, sizeof(Count), abortHandler);

        for (size_t i = 0; i < Count; ++i)
        {
            D2D1_GRADIENT_STOP gs = { };

            stream->read(&gs.position, sizeof(gs.position), abortHandler);
            stream->read(&gs.color, sizeof(gs.color), abortHandler);

            _CustomGradientStops_Deprecated.push_back(gs);
        }

        stream->read(&_XTextColor_Deprecated, sizeof(_XTextColor_Deprecated), abortHandler);
        stream->read(&_XLineColor_Deprecated, sizeof(_XLineColor_Deprecated), abortHandler);
        stream->read(&_YTextColor_Deprecated, sizeof(_YTextColor_Deprecated), abortHandler);
        stream->read(&_YLineColor_Deprecated, sizeof(_YLineColor_Deprecated), abortHandler);
        stream->read(&_DarkBandColor_Deprecated, sizeof(_DarkBandColor_Deprecated), abortHandler);

        stream->read(&_AmplitudeStep_Deprecated, sizeof(_AmplitudeStep_Deprecated), abortHandler);

        stream->read(&_Channels_Deprecated, sizeof(_Channels_Deprecated), abortHandler);
        stream->read(&_ShowToolTipsAlways, sizeof(_ShowToolTipsAlways), abortHandler);

        stream->read(&_WindowFunction, sizeof(_WindowFunction), abortHandler);
        stream->read(&_WindowParameter, sizeof(_WindowParameter), abortHandler);
        stream->read(&_WindowSkew, sizeof(_WindowSkew), abortHandler);

        if (Version >= 8)
        {
            stream->read(&_UseCustomBackColor_Deprecated, sizeof(_UseCustomBackColor_Deprecated), abortHandler);
            stream->read(&_UseCustomXTextColor_Deprecated, sizeof(_UseCustomXTextColor_Deprecated), abortHandler);
            stream->read(&_UseCustomXLineColor_Deprecated, sizeof(_UseCustomXLineColor_Deprecated), abortHandler);
            stream->read(&_UseCustomYTextColor_Deprecated, sizeof(_UseCustomYTextColor_Deprecated), abortHandler);
            stream->read(&_UseCustomYLineColor_Deprecated, sizeof(_UseCustomYLineColor_Deprecated), abortHandler);

            stream->read(&_LEDMode, sizeof(_LEDMode), abortHandler);

            stream->read(&_HorizontalGradient_Deprecated, sizeof(_HorizontalGradient_Deprecated), abortHandler);

            stream->read(&_LightBandColor_Deprecated, sizeof(_LightBandColor_Deprecated), abortHandler);
        }

        if (Version >= 9)
        {
            stream->read(&_PageIndex, sizeof(_PageIndex), abortHandler);
            stream->read(&_VisualizationType, sizeof(_VisualizationType), abortHandler);
            stream->read(&_LineWidth_Deprecated, sizeof(_LineWidth_Deprecated), abortHandler);
            stream->read(&_AreaOpacity_Deprecated, sizeof(_AreaOpacity_Deprecated), abortHandler);
        }

        if (Version >= 10)
        {
            stream->read(&_BackgroundMode_Deprecated, sizeof(_BackgroundMode_Deprecated), abortHandler); _BackgroundMode_Deprecated = std::clamp(_BackgroundMode_Deprecated, BackgroundMode::None, BackgroundMode::Artwork);

            _ShowArtworkOnBackground = (_BackgroundMode_Deprecated == BackgroundMode::Artwork);

            stream->read(&_ArtworkOpacity, sizeof(_ArtworkOpacity), abortHandler);

            stream->read(&_NumArtworkColors, sizeof(_NumArtworkColors), abortHandler);
            stream->read(&_LightnessThreshold, sizeof(_LightnessThreshold), abortHandler);
            stream->read(&_ColorOrder, sizeof(_ColorOrder), abortHandler);
        }

        if (Version >= 11)
        {
            stream->read(&_WeightingType, sizeof(_WeightingType), abortHandler);

            stream->read(&_SlopeFunctionOffset, sizeof(_SlopeFunctionOffset), abortHandler);

            stream->read(&_Slope, sizeof(_Slope), abortHandler);
            stream->read(&_SlopeOffset, sizeof(_SlopeOffset), abortHandler);

            stream->read(&_EqualizeAmount, sizeof(_EqualizeAmount), abortHandler);
            stream->read(&_EqualizeOffset, sizeof(_EqualizeOffset), abortHandler);
            stream->read(&_EqualizeDepth, sizeof(_EqualizeDepth), abortHandler);

            stream->read(&_WeightingAmount, sizeof(_WeightingAmount), abortHandler);

            stream->read(&_LineColor_Deprecated, sizeof(_LineColor_Deprecated), abortHandler);
            stream->read(&_UseCustomLineColor_Deprecated, sizeof(_UseCustomLineColor_Deprecated), abortHandler);
            stream->read(&_PeakLineColor_Deprecated, sizeof(_PeakLineColor_Deprecated), abortHandler);
            stream->read(&_UseCustomPeakLineColor_Deprecated, sizeof(_UseCustomPeakLineColor_Deprecated), abortHandler);
        }

        if (Version >= 12)
        {
            stream->read(&_BandwidthOffset, sizeof(_BandwidthOffset), abortHandler);
            stream->read(&_BandwidthCap, sizeof(_BandwidthCap), abortHandler);
            stream->read(&_BandwidthAmount, sizeof(_BandwidthAmount), abortHandler);
            stream->read(&_UseGranularBandwidth, sizeof(_UseGranularBandwidth), abortHandler);

            stream->read(&_KernelShape, sizeof(_KernelShape), abortHandler);
            stream->read(&_KernelShapeParameter, sizeof(_KernelShapeParameter), abortHandler);
            stream->read(&_KernelAsymmetry, sizeof(_KernelAsymmetry), abortHandler);
        }

        if (Version <= 13)
            ConvertColorSettings();

        if (Version >= 13)
        {
            pfc::string Path = stream->read_string(abortHandler);

            _ArtworkFilePath = pfc::wideFromUTF8(Path);
        }

        if (Version >= 14)
            _StyleManager.Read(stream, size, abortHandler);

        if (Version >= 16)
        {
            stream->read_object_t(_ReactionAlignment, abortHandler);

            stream->read_object_t(_XAxisTop_Deprecated, abortHandler);
            stream->read_object_t(_XAxisBottom_Deprecated, abortHandler);
            stream->read_object_t(_YAxisLeft_Deprecated, abortHandler);
            stream->read_object_t(_YAxisRight_Deprecated, abortHandler);

            stream->read_object_t(_FilterBankOrder, abortHandler);
            stream->read_object_t(_TimeResolution, abortHandler);
            stream->read_object_t(_IIRBandwidth, abortHandler);

            stream->read_object_t(_SuppressMirrorImage, abortHandler);
        }

        if (Version <= 17)
            ConvertGraphDescription();

        if (Version >= 17)
        {
            stream->read(&_FitMode, sizeof(_FitMode), abortHandler);
        }

        if (Version >= 18)
        {
            stream->read_object_t(_ShowArtworkOnBackground, abortHandler);

            stream->read_object_t(_GridRowCount, abortHandler);
            stream->read_object_t(_GridColumnCount, abortHandler);

            uint32_t GraphDescriptionVersion;

            stream->read_object_t(GraphDescriptionVersion, abortHandler);

            stream->read_object_t(_VerticalLayout, abortHandler);

            _GraphDescriptions.clear();

            stream->read_object_t(Count, abortHandler);

            for (size_t i = 0; i < Count; ++i)
            {
                graph_description_t gd;

                pfc::string Description; stream->read_string(Description, abortHandler); gd._Description = pfc::wideFromUTF8(Description);

                stream->read_object_t(gd._SelectedChannels, abortHandler);
                stream->read_object_t(gd._FlipHorizontally, abortHandler);
                stream->read_object_t(gd._FlipVertically, abortHandler);

                stream->read_object(&gd._XAxisMode, sizeof(gd._XAxisMode), abortHandler);
                stream->read_object_t(gd._XAxisTop, abortHandler);
                stream->read_object_t(gd._XAxisBottom, abortHandler);

                stream->read_object(&gd._YAxisMode, sizeof(gd._YAxisMode), abortHandler);
                stream->read_object_t(gd._YAxisLeft, abortHandler);
                stream->read_object_t(gd._YAxisRight, abortHandler);

                stream->read_object_t(gd._AmplitudeLo, abortHandler);
                stream->read_object_t(gd._AmplitudeHi, abortHandler);
                stream->read_object_t(gd._AmplitudeStep, abortHandler);

                stream->read_object_t(gd._UseAbsolute, abortHandler);
                stream->read_object_t(gd._Gamma, abortHandler);

                stream->read_object_t(gd._HRatio, abortHandler);
                stream->read_object_t(gd._VRatio, abortHandler);

                if (GraphDescriptionVersion > 1)
                {
                    stream->read_object_t(gd._LPadding, abortHandler);
                    stream->read_object_t(gd._RPadding, abortHandler);
                    stream->read_object_t(gd._TPadding, abortHandler);
                    stream->read_object_t(gd._BPadding, abortHandler);

                    stream->read_object(&gd._HAlignment, sizeof(gd._HAlignment), abortHandler);
                    stream->read_object(&gd._VAlignment, sizeof(gd._VAlignment), abortHandler);
                }

                if (GraphDescriptionVersion > 2)
                {
                    stream->read_object(&gd._HorizontalAlignment, sizeof(gd._HorizontalAlignment), abortHandler);
                    stream->read_object(&gd._VerticalAlignment, sizeof(gd._VerticalAlignment), abortHandler);
                }

                if (GraphDescriptionVersion > 3) // v0.10.0.0-alpha5
                {
                    stream->read_object_t(gd._SwapChannels, abortHandler);
                }

                _GraphDescriptions.push_back(gd);
            }
        }

        if (Version >= 19)
        {
            stream->read_object_t(_ConstantQ, abortHandler);
            stream->read_object_t(_CompensateBandwidth, abortHandler);
            stream->read_object_t(_UsePreWarpedQ, abortHandler);
        }

        if (Version >= 20)
        {
            pfc::string Path;

            stream->read_string(Path, abortHandler); 

            if (!isPreset)
                _PresetsDirectoryPath = pfc::wideFromUTF8(Path);
        }

        if (Version >= 21)
        {
            stream->read_object_t(_IsScrollingSpectrogram, abortHandler);
        }

        if (Version >= 22)
        {
            stream->read_object_t(_IsHorizontalPeakMeter, abortHandler);
        }

        if (Version >= 23)
        {
            stream->read_object_t(_LEDLight, abortHandler);
            stream->read_object_t(_LEDGap, abortHandler);
        }

        if (Version >= 24)
        {
            stream->read_object_t(_FitWindow, abortHandler);
        }

        if (Version >= 25)
        {
            stream->read_object_t(_RMSWindow, abortHandler);
        }

        if (Version >= 26)
        {
            stream->read_object_t(_BarGap, abortHandler);
            stream->read_object_t(_HasRMSPlus3, abortHandler);
        }

        if (Version >= 27)
        {
            stream->read(&_ChannelPair, sizeof(_ChannelPair), abortHandler);
            _ChannelPair = std::clamp(_ChannelPair, ChannelPair::FrontLeftRight, ChannelPair::TopBackLeftRight);

            stream->read_object_t(_IsHorizontalLevelMeter, abortHandler);
            stream->read_object_t(_IsHorizontalSpectrogram, abortHandler);
            stream->read_object_t(_UseSpectrumBarMetrics, abortHandler);
        }

        if (Version >= 28)
        {
            stream->read_object_t(_InnerRadius, abortHandler);
            stream->read_object_t(_OuterRadius, abortHandler);
            stream->read_object_t(_AngularVelocity, abortHandler);
        }

        if (Version >= 29)
        {
            stream->read(&_ArtworkType, sizeof(_ArtworkType), abortHandler);
        }

        if (Version >= 30)
        {
            stream->read(&_LEDIntegralSize, sizeof(_LEDIntegralSize), abortHandler);
        }

        if (Version >= 31)
        {
            stream->read_object_t(_XYMode, abortHandler);
            stream->read_object_t(_XGain, abortHandler);
            stream->read_object_t(_YGain, abortHandler);

            stream->read_object_t(_HasPhosphorDecay, abortHandler);
            stream->read_object_t(_BlurSigma, abortHandler);
            stream->read_object_t(_DecayFactor, abortHandler);
        }

        if (Version >= 32)
        {
            stream->read_object_t(_HasCenterScale, abortHandler);
            stream->read_object_t(_MaxBarSize, abortHandler);
        }

        if (Version >= 33)
        {
            stream->read_object_t(_VisualizeDuringPause, abortHandler);
            stream->read_object_t(_HasScaleLines, abortHandler);
        }

        if (Version >= 34)
        {
            stream->read_object_t(_Rotation, abortHandler);
        }

        if (Version >= 35)
        {
            stream->read_object_t(_OpacityMode, abortHandler);
        }
    }
    catch (exception & ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to read configuration: %s", ex.what());

        Reset();
    }
}

/// <summary>
/// Writes this instance to the specified writer.
/// </summary>
void state_t::Write(stream_writer * stream, abort_callback & abortHandler, bool isPreset) const noexcept
{
    try
    {
        stream->write(&_CurrentVersion, sizeof(_CurrentVersion), abortHandler);

        #pragma region User Interface

        stream->write(&_Bounds, sizeof(_Bounds), abortHandler);

        stream->write(&_RefreshRateLimit, sizeof(_RefreshRateLimit), abortHandler);

        stream->write(&_UseHardwareRendering, sizeof(_UseHardwareRendering), abortHandler);
        stream->write(&_UseAntialiasing, sizeof(_UseAntialiasing), abortHandler);

        stream->write(&_UseZeroTrigger_Deprecated, sizeof(_UseZeroTrigger_Deprecated), abortHandler);
        stream->write(&_WindowDuration, sizeof(_WindowDuration), abortHandler);

        #pragma endregion

        stream->write(&_TransformMethod, sizeof(_TransformMethod), abortHandler);

        #pragma region FFT

        stream->write(&_FFTMode, sizeof(_FFTMode), abortHandler);
        stream->write(&_FFTCustom, sizeof(_FFTCustom), abortHandler);
        stream->write(&_FFTDuration, sizeof(_FFTDuration), abortHandler);
        stream->write(&_MappingMethod, sizeof(_MappingMethod), abortHandler);

        stream->write(&_SmoothingMethod, sizeof(_SmoothingMethod), abortHandler);
        stream->write(&_SmoothingFactor, sizeof(_SmoothingFactor), abortHandler);
        stream->write(&_KernelSize, sizeof(_KernelSize), abortHandler);
        stream->write(&_AggregationMethod, sizeof(_AggregationMethod), abortHandler);
        stream->write(&_SmoothLowerFrequencies, sizeof(_SmoothLowerFrequencies), abortHandler);
        stream->write(&_SmoothGainTransition, sizeof(_SmoothGainTransition), abortHandler);

        #pragma endregion

        #pragma region Frequencies

        stream->write(&_FrequencyDistribution, sizeof(_FrequencyDistribution), abortHandler);

        stream->write(&_BandCount, sizeof(_BandCount), abortHandler);
        stream->write(&_LoFrequency, sizeof(_LoFrequency), abortHandler);
        stream->write(&_HiFrequency, sizeof(_HiFrequency), abortHandler);

        stream->write(&_LoNote, sizeof(_LoNote), abortHandler);
        stream->write(&_HiNote, sizeof(_HiNote), abortHandler);
        stream->write(&_BandsPerOctave, sizeof(_BandsPerOctave), abortHandler);
        stream->write(&_TuningPitch, sizeof(_TuningPitch), abortHandler);
        stream->write(&_Transpose, sizeof(_Transpose), abortHandler);

        stream->write(&_ScalingFunction, sizeof(_ScalingFunction), abortHandler);
        stream->write(&_SkewFactor, sizeof(_SkewFactor), abortHandler);
        stream->write(&_Bandwidth, sizeof(_Bandwidth), abortHandler);

        #pragma endregion

        #pragma region Rendering

        stream->write(&_BackColor_Deprecated, sizeof(_BackColor_Deprecated), abortHandler);

        stream->write(&_XAxisMode_Deprecated, sizeof(_XAxisMode_Deprecated), abortHandler);

        stream->write(&_YAxisMode_Deprecated, sizeof(_YAxisMode_Deprecated), abortHandler);

        stream->write(&_AmplitudeLo_Deprecated, sizeof(_AmplitudeLo_Deprecated), abortHandler);
        stream->write(&_AmplitudeHi_Deprecated, sizeof(_AmplitudeHi_Deprecated), abortHandler);
        stream->write(&_UseAbsolute_Deprecated, sizeof(_UseAbsolute_Deprecated), abortHandler);
        stream->write(&_Gamma_Deprecated, sizeof(_Gamma_Deprecated), abortHandler);

        stream->write(&_ColorScheme_Deprecated, sizeof(_ColorScheme_Deprecated), abortHandler);

        stream->write(&_DrawBandBackground_Deprecated, sizeof(_DrawBandBackground_Deprecated), abortHandler);

        stream->write(&_PeakMode, sizeof(_PeakMode), abortHandler);
        stream->write(&_HoldTime, sizeof(_HoldTime), abortHandler);
        stream->write(&_Acceleration, sizeof(_Acceleration), abortHandler);

        #pragma endregion

        size_t Size = _CustomGradientStops_Deprecated.size();

        stream->write(&Size, sizeof(Size), abortHandler);

        for (const auto & Iter : _CustomGradientStops_Deprecated)
        {
            stream->write(&Iter.position, sizeof(Iter.position), abortHandler);
            stream->write(&Iter.color, sizeof(Iter.color), abortHandler);
        }

        stream->write(&_XTextColor_Deprecated, sizeof(_XTextColor_Deprecated), abortHandler);
        stream->write(&_XLineColor_Deprecated, sizeof(_XLineColor_Deprecated), abortHandler);
        stream->write(&_YTextColor_Deprecated, sizeof(_YTextColor_Deprecated), abortHandler);
        stream->write(&_YLineColor_Deprecated, sizeof(_YLineColor_Deprecated), abortHandler);
        stream->write(&_DarkBandColor_Deprecated, sizeof(_DarkBandColor_Deprecated), abortHandler);

        stream->write(&_AmplitudeStep_Deprecated, sizeof(_AmplitudeStep_Deprecated), abortHandler);

        stream->write(&_Channels_Deprecated, sizeof(_Channels_Deprecated), abortHandler);
        stream->write(&_ShowToolTipsAlways, sizeof(_ShowToolTipsAlways), abortHandler);

        stream->write(&_WindowFunction, sizeof(_WindowFunction), abortHandler);
        stream->write(&_WindowParameter, sizeof(_WindowParameter), abortHandler);
        stream->write(&_WindowSkew, sizeof(_WindowSkew), abortHandler);

        // Version 8
        stream->write(&_UseCustomBackColor_Deprecated,  sizeof(_UseCustomBackColor_Deprecated), abortHandler);
        stream->write(&_UseCustomXTextColor_Deprecated, sizeof(_UseCustomXTextColor_Deprecated), abortHandler);
        stream->write(&_UseCustomXLineColor_Deprecated, sizeof(_UseCustomXLineColor_Deprecated), abortHandler);
        stream->write(&_UseCustomYTextColor_Deprecated, sizeof(_UseCustomYTextColor_Deprecated), abortHandler);
        stream->write(&_UseCustomYLineColor_Deprecated, sizeof(_UseCustomYLineColor_Deprecated), abortHandler);

        stream->write(&_LEDMode, sizeof(_LEDMode), abortHandler);

        stream->write(&_HorizontalGradient_Deprecated, sizeof(_HorizontalGradient_Deprecated), abortHandler);

        stream->write(&_LightBandColor_Deprecated, sizeof(_LightBandColor_Deprecated), abortHandler);

        // Version 9
        stream->write(&_PageIndex, sizeof(_PageIndex), abortHandler);
        stream->write(&_VisualizationType, sizeof(_VisualizationType), abortHandler);
        stream->write(&_LineWidth_Deprecated, sizeof(_LineWidth_Deprecated), abortHandler);
        stream->write(&_AreaOpacity_Deprecated, sizeof(_AreaOpacity_Deprecated), abortHandler);

        // Version 10
        stream->write(&_BackgroundMode_Deprecated, sizeof(_BackgroundMode_Deprecated), abortHandler);
        stream->write(&_ArtworkOpacity, sizeof(_ArtworkOpacity), abortHandler);

        stream->write(&_NumArtworkColors, sizeof(_NumArtworkColors), abortHandler);
        stream->write(&_LightnessThreshold, sizeof(_LightnessThreshold), abortHandler);
        stream->write(&_ColorOrder, sizeof(_ColorOrder), abortHandler);

        // Version 11
        stream->write(&_WeightingType, sizeof(_WeightingType), abortHandler);

        stream->write(&_SlopeFunctionOffset, sizeof(_SlopeFunctionOffset), abortHandler);

        stream->write(&_Slope, sizeof(_Slope), abortHandler);
        stream->write(&_SlopeOffset, sizeof(_SlopeOffset), abortHandler);

        stream->write(&_EqualizeAmount, sizeof(_EqualizeAmount), abortHandler);
        stream->write(&_EqualizeOffset, sizeof(_EqualizeOffset), abortHandler);
        stream->write(&_EqualizeDepth, sizeof(_EqualizeDepth), abortHandler);

        stream->write(&_WeightingAmount, sizeof(_WeightingAmount), abortHandler);

        stream->write(&_LineColor_Deprecated, sizeof(_LineColor_Deprecated), abortHandler);
        stream->write(&_UseCustomLineColor_Deprecated, sizeof(_UseCustomLineColor_Deprecated), abortHandler);
        stream->write(&_PeakLineColor_Deprecated, sizeof(_PeakLineColor_Deprecated), abortHandler);
        stream->write(&_UseCustomPeakLineColor_Deprecated, sizeof(_UseCustomPeakLineColor_Deprecated), abortHandler);

        // Version 12
        stream->write(&_BandwidthOffset, sizeof(_BandwidthOffset), abortHandler);
        stream->write(&_BandwidthCap, sizeof(_BandwidthCap), abortHandler);
        stream->write(&_BandwidthAmount, sizeof(_BandwidthAmount), abortHandler);
        stream->write(&_UseGranularBandwidth, sizeof(_UseGranularBandwidth), abortHandler);

        stream->write(&_KernelShape, sizeof(_KernelShape), abortHandler);
        stream->write(&_KernelShapeParameter, sizeof(_KernelShapeParameter), abortHandler);
        stream->write(&_KernelAsymmetry, sizeof(_KernelAsymmetry), abortHandler);

        // Version 13
        {
            pfc::string Path = pfc::utf8FromWide(_ArtworkFilePath.c_str());

            stream->write_string(Path, abortHandler);
        }

        // Version 15
        _StyleManager.Write(stream, abortHandler);

        // Version 16
        stream->write_object_t(_ReactionAlignment, abortHandler);

        stream->write_object_t(_XAxisTop_Deprecated, abortHandler);
        stream->write_object_t(_XAxisBottom_Deprecated, abortHandler);
        stream->write_object_t(_YAxisLeft_Deprecated, abortHandler);
        stream->write_object_t(_YAxisRight_Deprecated, abortHandler);

        stream->write_object_t(_FilterBankOrder, abortHandler);
        stream->write_object_t(_TimeResolution, abortHandler);
        stream->write_object_t(_IIRBandwidth, abortHandler);

        stream->write_object_t(_SuppressMirrorImage, abortHandler);

        // Version 17
        stream->write(&_FitMode, sizeof(_FitMode), abortHandler);

        // Version 18, v0.7.1.0-beta-2
        stream->write_object_t(_ShowArtworkOnBackground, abortHandler);

        stream->write_object_t(_GridRowCount, abortHandler);
        stream->write_object_t(_GridColumnCount, abortHandler);

        stream->write_object_t(graph_description_t::_CurrentVersion, abortHandler);

        stream->write_object_t(_VerticalLayout, abortHandler);

        stream->write_object_t(_GraphDescriptions.size(), abortHandler);

        for (auto & gd : _GraphDescriptions)
        {
            pfc::string Description = pfc::utf8FromWide(gd._Description.c_str());
            stream->write_string(Description, abortHandler);

            stream->write_object_t(gd._SelectedChannels, abortHandler);
            stream->write_object_t(gd._FlipHorizontally, abortHandler);
            stream->write_object_t(gd._FlipVertically, abortHandler);

            stream->write_object (&gd._XAxisMode, sizeof(gd._XAxisMode), abortHandler);
            stream->write_object_t(gd._XAxisTop, abortHandler);
            stream->write_object_t(gd._XAxisBottom, abortHandler);

            stream->write_object (&gd._YAxisMode, sizeof(gd._YAxisMode), abortHandler);
            stream->write_object_t(gd._YAxisLeft, abortHandler);
            stream->write_object_t(gd._YAxisRight, abortHandler);

            stream->write_object_t(gd._AmplitudeLo, abortHandler);
            stream->write_object_t(gd._AmplitudeHi, abortHandler);
            stream->write_object_t(gd._AmplitudeStep, abortHandler);

            stream->write_object_t(gd._UseAbsolute, abortHandler);
            stream->write_object_t(gd._Gamma, abortHandler);

            stream->write_object_t(gd._HRatio, abortHandler);
            stream->write_object_t(gd._VRatio, abortHandler);

            // Version 2, v0.7.6.0
            if (graph_description_t::_CurrentVersion > 1)
            {
                stream->write_object_t(gd._LPadding, abortHandler);
                stream->write_object_t(gd._RPadding, abortHandler);
                stream->write_object_t(gd._TPadding, abortHandler);
                stream->write_object_t(gd._BPadding, abortHandler);

                stream->write_object(&gd._HAlignment, sizeof(gd._HAlignment), abortHandler);
                stream->write_object(&gd._VAlignment, sizeof(gd._VAlignment), abortHandler);
            }

            // Version 3, v0.8.0.0-beta2
            if (graph_description_t::_CurrentVersion > 2)
            {
                stream->write_object(&gd._HorizontalAlignment, sizeof(gd._HorizontalAlignment), abortHandler); // v30 adds HorizontalAlignment::Fit
                stream->write_object(&gd._VerticalAlignment, sizeof(gd._VerticalAlignment), abortHandler);
            }

            // Version 4, v0.10.0.0-alpha5
            if (graph_description_t::_CurrentVersion > 2)
            {
                stream->write_object_t(gd._SwapChannels, abortHandler);
            }

            // Version 5, v0.11.0.0-alpha1
            if (graph_description_t::_CurrentVersion > 4)
            {
                stream->write_object_t(gd._XAxisDecimals, abortHandler);
            }
        }

        // Version 19, v0.7.2.0
        stream->write_object_t(_ConstantQ, abortHandler);
        stream->write_object_t(_CompensateBandwidth, abortHandler);
        stream->write_object_t(_UsePreWarpedQ, abortHandler);

        // Version 20
        {
            pfc::string Path;

            if (!isPreset)
                Path = pfc::utf8FromWide(_PresetsDirectoryPath.c_str());

            stream->write_string(Path, abortHandler);
        }

        // Version 21, v0.7.5.0-beta1
        stream->write_object_t(_IsScrollingSpectrogram, abortHandler);

        // Version 22, v0.7.5.0-beta2
        stream->write_object_t(_IsHorizontalPeakMeter, abortHandler);

        // Version 23, v0.7.5.0-beta3
        stream->write_object_t(_LEDLight, abortHandler);
        stream->write_object_t(_LEDGap, abortHandler);

        // Version 24, v0.7.5.2
        stream->write_object_t(_FitWindow, abortHandler);

        // Version 25, v0.7.5.3
        stream->write_object_t(_RMSWindow, abortHandler);

        // Version 26, v0.7.6.0
        stream->write_object_t(_BarGap, abortHandler);
        stream->write_object_t(_HasRMSPlus3, abortHandler);

        // Version 27, v0.8.0.0-beta1
        stream->write_object(&_ChannelPair, sizeof(_ChannelPair), abortHandler);
        stream->write_object_t(_IsHorizontalLevelMeter, abortHandler);
        stream->write_object_t(_IsHorizontalSpectrogram, abortHandler);
        stream->write_object_t(_UseSpectrumBarMetrics, abortHandler);

        // Version 28, v0.8.0.0-beta2
        stream->write_object_t(_InnerRadius, abortHandler);
        stream->write_object_t(_OuterRadius, abortHandler);
        stream->write_object_t(_AngularVelocity, abortHandler);

        // Version 29, v0.8.0.0
        stream->write(&_ArtworkType, sizeof(_ArtworkType), abortHandler);

        // Version 30, v0.9.0.0-alpha2
        stream->write(&_LEDIntegralSize, sizeof(_LEDIntegralSize), abortHandler);

        // Version 31, v0.9.0.0-alpha3
        stream->write_object_t(_XYMode, abortHandler);
        stream->write_object_t(_XGain, abortHandler);
        stream->write_object_t(_YGain, abortHandler);
        stream->write_object_t(_HasPhosphorDecay, abortHandler);
        stream->write_object_t(_BlurSigma, abortHandler);
        stream->write_object_t(_DecayFactor, abortHandler);

        // Version 32, v0.9.2
        stream->write_object_t(_HasCenterScale, abortHandler);
        stream->write_object_t(_MaxBarSize, abortHandler);

        // Version 33, v0.10.0-alpha4
        stream->write_object_t(_VisualizeDuringPause, abortHandler);
        stream->write_object_t(_HasScaleLines, abortHandler);

        // Version 34, v0.10.0-alpha5
        stream->write_object_t(_Rotation, abortHandler);

        // Version 35, v0.10.0-beta1
        stream->write_object_t(_OpacityMode, abortHandler);
    }
    catch (exception & ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to write configuration: %s", ex.what());
    }
}

/// <summary>
/// Deserializes this instance from a JSON character array.
/// </summary>
void state_t::FromJSON(const char * data, size_t size, bool isPreset)
{
    json Object = json::parse(data, data + size, nullptr, true);

    // User Interface
    _RefreshRateLimit = Object.value("refreshRateLimit", _RefreshRateLimit);

    // Configuration Dialog
    const auto & Dialog = Object.value("configurationDialog", json::object());

    const auto & Bounds = Dialog.value("bounds", json::object());

    _Bounds.left   = Bounds.value("left", _Bounds.left);
    _Bounds.top    = Bounds.value("top", _Bounds.top);
    _Bounds.right  = Bounds.value("right", _Bounds.right);
    _Bounds.bottom = Bounds.value("bottom", _Bounds.bottom);

    _PageIndex = Dialog.value("page", _PageIndex);

    // Visalization
    _VisualizationType = Object.value("visualizationType", _VisualizationType);

    const auto & PeakIndicators = Object.value("peakIndicators", json::object());

    _PeakMode     = PeakIndicators.value("mode", _PeakMode);
    _HoldTime     = PeakIndicators.value("holdTime", _HoldTime);
    _Acceleration = PeakIndicators.value("acceleration", _Acceleration);

    const auto & LEDs = Object.value("leds", json::object());

    _LEDMode         = LEDs.value("enabled", _LEDMode);
    _LEDLight        = LEDs.value("lightSize", _LEDLight);
    _LEDGap          = LEDs.value("gapSize", _LEDGap);
    _LEDIntegralSize = LEDs.value("integralSize", _LEDIntegralSize);

    const auto & Radial = Object.value("radial", json::object());

    _InnerRadius     = Radial.value("innerRadius", _InnerRadius);
    _OuterRadius     = Radial.value("outerRadius", _OuterRadius);
    _AngularVelocity = Radial.value("angularVelocity", _AngularVelocity);

    const auto & Spectrogram = Object.value("spectrogram", json::object());

    _IsScrollingSpectrogram  = Spectrogram.value("scrolling", _IsScrollingSpectrogram);
    _IsHorizontalSpectrogram = Spectrogram.value("horizontally", _IsHorizontalSpectrogram);
    _UseSpectrumBarMetrics   = Spectrogram.value("useBarMetrics", _UseSpectrumBarMetrics);

    const auto & PeakMeter = Object.value("peakMeter", json::object());

    _RMSWindow             = PeakMeter.value("rmsWindow", _RMSWindow);
    _IsHorizontalPeakMeter = PeakMeter.value("horizontally", _IsHorizontalPeakMeter);
    _HasCenterScale        = PeakMeter.value("hasCenterScale", _HasCenterScale);
    _HasRMSPlus3           = PeakMeter.value("hasRMSPlus3", _HasRMSPlus3);
    _HasScaleLines         = PeakMeter.value("hasScaleLines", _HasScaleLines);
    _BarGap                = PeakMeter.value("barGap", _BarGap);
    _MaxBarSize            = PeakMeter.value("maxBarSize", _MaxBarSize);

    const auto & LevelMeter = Object.value("levelMeter", json::object());

    _IsHorizontalLevelMeter = LevelMeter.value("horizontally", _IsHorizontalLevelMeter);

    const auto & Oscilloscope = Object.value("oscilloscope", json::object());

    _XYMode   = Oscilloscope.value("xyMode", _XYMode);
    _XGain    = Oscilloscope.value("xGain", _XGain);
    _YGain    = Oscilloscope.value("yGain", _YGain);
    _Rotation = Oscilloscope.value("rotation", _Rotation);

    const auto & PhosporDecay = Oscilloscope.value("phosphorDecay", json::object());

    _HasPhosphorDecay = PhosporDecay.value("enabled", _HasPhosphorDecay);
    _BlurSigma        = PhosporDecay.value("blurSigma", _BlurSigma);
    _DecayFactor      = PhosporDecay.value("decayFactor", _DecayFactor);

    const auto & BitMeter = Object.value("bitMeter", json::object());

    _OpacityMode = BitMeter.value("opacityMode", _OpacityMode);

    // Transform
    const auto & Transform = Object.value("transform", json::object());

    _TransformMethod        = Transform.value("method", _TransformMethod);

    // FFT
    _WindowFunction         = Transform.value("windowFunction", _WindowFunction);
    _WindowParameter        = Transform.value("windowParameter", _WindowParameter);
    _WindowSkew             = Transform.value("windowSkew", _WindowSkew);
    _ReactionAlignment      = Transform.value("reactionAlignment", _ReactionAlignment);

    _FFTMode                = Transform.value("mode", _FFTMode);
    _FFTCustom              = Transform.value("custom", _FFTCustom);
    _FFTDuration            = Transform.value("duration", _FFTDuration);

    _AggregationMethod      = Transform.value("aggregationMethod", _AggregationMethod);
    _MappingMethod          = Transform.value("mapping", _MappingMethod);
    _SmoothLowerFrequencies = Transform.value("smoothLowerFrequencies", _SmoothLowerFrequencies);
    _SmoothGainTransition   = Transform.value("smoothGainTransition", _SmoothGainTransition);

    _KernelSize             = Transform.value("kernelSize", _KernelSize);

    // CQT
    const auto & CQT = Transform.value("cqt", json::object());

    _BandwidthOffset      = CQT.value("bandwidthOffset", _BandwidthOffset);
    _BandwidthCap         = CQT.value("bandwidthCap", _BandwidthCap);
    _BandwidthAmount      = CQT.value("bandwidthAmount", _BandwidthAmount);
    _UseGranularBandwidth = CQT.value("useGranularBandwidth", _UseGranularBandwidth);

    _KernelShape          = CQT.value("kernelShape", _KernelShape);
    _KernelShapeParameter = CQT.value("kernelShapeParameter", _KernelShapeParameter);
    _KernelAsymmetry      = CQT.value("kernelAsymmetry", _KernelAsymmetry); 

    // IIR (SWIFT / Analog-style)
    const auto & IIR = Transform.value("iir", json::object());

    _FilterBankOrder     = IIR.value("filterBankOrder", _FilterBankOrder);
    _TimeResolution      = IIR.value("timeResolution", _TimeResolution);
    _IIRBandwidth        = IIR.value("bandwidth", _IIRBandwidth);
    _ConstantQ           = IIR.value("constantQ", _ConstantQ);
    _CompensateBandwidth = IIR.value("compensateBandwidth", _CompensateBandwidth);
    _UsePreWarpedQ       = IIR.value("usePreWarpedQ", _UsePreWarpedQ);

    // Frequencies
    const auto & Frequencies = Object.value("frequencies", json::object());

    _FrequencyDistribution = Frequencies.value("distribution", _FrequencyDistribution);
    _BandCount             = Frequencies.value("bandCount", _BandCount);

    _LoFrequency           = Frequencies.value("loFrequency", _LoFrequency);
    _HiFrequency           = Frequencies.value("hiFrequency", _HiFrequency);

    _LoNote                = Frequencies.value("loNote", _LoNote);
    _HiNote                = Frequencies.value("hiNote", _HiNote);

    _BandsPerOctave        = Frequencies.value("bandsPerOctave", _BandsPerOctave);
    _TuningPitch           = Frequencies.value("tuningPitch", _TuningPitch);
    _Transpose             = Frequencies.value("transpose", _Transpose);

    _ScalingFunction       = Frequencies.value("scalingFunction", _ScalingFunction);
    _SkewFactor            = Frequencies.value("skewFactor", _SkewFactor);
    _Bandwidth             = Frequencies.value("bandwidth", _Bandwidth);

    // Acoustic Filters
    const auto & Filters = Object.value("acousticFilters", json::object());

    _WeightingType = Filters.value("weightingType", _WeightingType);

    _SlopeFunctionOffset = Filters.value("slopeFunctionOffset", _SlopeFunctionOffset);
    _Slope               = Filters.value("slope", _Slope);
    _SlopeOffset         = Filters.value("slopeOffset", _SlopeOffset);

    _EqualizeAmount      = Filters.value("equalizeAmount", _EqualizeAmount);
    _EqualizeOffset      = Filters.value("equalizeOffset", _EqualizeOffset);
    _EqualizeDepth       = Filters.value("equalizeDepth", _EqualizeDepth);

    _WeightingAmount     = Filters.value("weightingAmount", _WeightingAmount);

    // Common
    _SmoothingMethod      = Object.value("smoothingMethod", _SmoothingMethod);
    _SmoothingFactor      = Object.value("smoothingFactor", _SmoothingFactor);

    _ShowToolTipsAlways   = Object.value("showToolTipsAlways", _ShowToolTipsAlways);
    _SuppressMirrorImage  = Object.value("suppressMirrorImage", _SuppressMirrorImage);
    _VisualizeDuringPause = Object.value("visualizeDuringPause", _VisualizeDuringPause);

    // Artwork
    const auto & Artwork = Object.value("artwork", json::object());

    _ArtworkType             = Artwork.value("type", _ArtworkType);

    _NumArtworkColors        = Artwork.value("colorCount", _NumArtworkColors);
    _LightnessThreshold      = Artwork.value("lightnessThreshold", _LightnessThreshold);
    _TransparencyThreshold   = Artwork.value("transparencyThreshold", _TransparencyThreshold);

    _ColorOrder              = Artwork.value("colorOrder", _ColorOrder);

    _ShowArtworkOnBackground = Artwork.value("showArtworkOnBackground", _ShowArtworkOnBackground);

    _FitMode                 = Artwork.value("fitMode", _FitMode);
    _FitWindow               = Artwork.value("fitWindow", _FitWindow);
    _ArtworkOpacity          = Artwork.value("opacity", _ArtworkOpacity);
    _ArtworkFilePath         = msc::UTF8ToWide(Artwork.value("filePath", msc::WideToUTF8(_ArtworkFilePath)));

    const auto & Grid = Object.value("grid", json::object());

    _GridRowCount    = Grid.value("rows", _GridRowCount);
    _GridColumnCount = Grid.value("columns", _GridColumnCount);
    _VerticalLayout  = Grid.value("verticalLayout", _VerticalLayout);

    {
        std::vector<graph_description_t> GraphDescriptions;

        const auto & Graphs = Grid.value("graphs", json::array_t());

        for (auto & Graph : Graphs)
            GraphDescriptions.push_back(graph_description_t::FromJSON(Graph));

        _GraphDescriptions = std::move(GraphDescriptions);
    }

    _ChannelPair = Object.value("channelPair", _ChannelPair);

    {
        const auto & Styles = Object.value("styles", json::array());

        _StyleManager.FromJSON(Styles);
    }

    if (!isPreset)
        _PresetsDirectoryPath = msc::UTF8ToWide(Object.value("presetsDirectory", msc::WideToUTF8(_PresetsDirectoryPath)));
}

/// <summary>
/// Serializes this instance to JSON string.
/// </summary>
json state_t::ToJSON(bool isPreset) const
{
    json Object =
    {
        { "schemaVersion", _SchemaVersion },

        // User Interface
        { "refreshRateLimit", _RefreshRateLimit },

        // Configuration Dialog
        json::object_t::value_type
        (
            "configurationDialog", json::object
            ({
                { "bounds", json::object
                    ({
                        { "left", _Bounds.left },
                        { "top", _Bounds.top },
                        { "right", _Bounds.right },
                        { "bottom", _Bounds.bottom }
                    })
                },
                { "page", _PageIndex },
            })
        ),

        // Visalization
        { "visualizationType", _VisualizationType },

        json::object_t::value_type
        (
            "peakIndicators", json::object
            ({
                { "mode", _PeakMode },
                { "holdTime", _HoldTime },
                { "acceleration", _Acceleration },
            })
        ),

        json::object_t::value_type
        (
            "leds", json::object
            ({
                { "enabled", _LEDMode },
                { "lightSize", _LEDLight },
                { "gapSize", _LEDGap },
                { "integralSize", _LEDIntegralSize },
            })
        ),

        json::object_t::value_type
        (
            "radial", json::object
            ({
                { "innerRadius", _InnerRadius },
                { "outerRadius", _OuterRadius },
                { "angularVelocity", _AngularVelocity },
            })
        ),

        json::object_t::value_type
        (
            "spectrogram", json::object
            ({
                { "scrolling", _IsScrollingSpectrogram },
                { "horizontally", _IsHorizontalSpectrogram },
                { "useBarMetrics", _UseSpectrumBarMetrics },
            })
        ),

        json::object_t::value_type
        (
            "peakMeter", json::object
            ({
                { "rmsWindow", _RMSWindow },
                { "horizontally", _IsHorizontalPeakMeter },
                { "hasCenterScale", _HasCenterScale },
                { "hasRMSPlus3", _HasRMSPlus3 },
                { "hasScaleLines", _HasScaleLines },
                { "barGap", _BarGap },
                { "maxBarSize", _MaxBarSize },
            })
        ),

        json::object_t::value_type
        (
            "levelMeter", json::object
            ({
                { "horizontally", _IsHorizontalLevelMeter },
            })
        ),

        json::object_t::value_type
        (
            "oscilloscope", json::object
            ({
                { "xyMode", _XYMode },
                { "xGain", _XGain },
                { "yGain", _YGain },

                { "rotation", _Rotation },

                { "phosphorDecay", json::object
                    ({
                        { "enabled", _HasPhosphorDecay },
                        { "blurSigma", _BlurSigma },
                        { "decayFactor", _DecayFactor },
                    })
                }
            })
        ),

        json::object_t::value_type
        (
            "bitMeter", json::object
            ({
                { "opacityMode", _OpacityMode },
            })
        ),

        // FFT
        json::object_t::value_type
        (
            "transform", json::object
            ({
                { "method", _TransformMethod },

                { "windowFunction", _WindowFunction },
                { "windowParameter", _WindowParameter },
                { "windowSkew", _WindowSkew },
                { "reactionAlignment", _ReactionAlignment },

                { "mode", _FFTMode },
                { "custom", _FFTCustom },
                { "duration", _FFTDuration },

                { "aggregationMethod", _AggregationMethod },
                { "mapping", _MappingMethod },
                { "smoothLowerFrequencies", _SmoothLowerFrequencies },
                { "smoothGainTransition", _SmoothGainTransition },

                { "kernelSize", _KernelSize },

                // CQT
                json::object_t::value_type
                (
                    "cqt", json::object
                    ({
                        { "bandwidthOffset", _BandwidthOffset },
                        { "bandwidthCap", _BandwidthCap },
                        { "bandwidthAmount", _BandwidthAmount },
                        { "useGranularBandwidth", _UseGranularBandwidth },

                        { "kernelShape", _KernelShape },
                        { "kernelShapeParameter", _KernelShapeParameter },
                        { "kernelAsymmetry", _KernelAsymmetry },

                    })
                ),

                // IIR (SWIFT / Analog-style)
                json::object_t::value_type
                (
                    "iir", json::object
                    ({
                        { "filterBankOrder", _FilterBankOrder },
                        { "timeResolution", _TimeResolution },
                        { "bandwidth", _IIRBandwidth },
                        { "constantQ", _ConstantQ },
                        { "compensateBandwidth", _CompensateBandwidth },
                        { "usePrewarpedQ", _UsePreWarpedQ },
                    })
                ),
            })
        ),

        // Frequencies
        json::object_t::value_type
        (
            "frequencies",
                json::object
                ({
                    { "distribution", _FrequencyDistribution },

                    { "bandCount", _BandCount },

                    { "loFrequency", _LoFrequency },
                    { "hiFrequency", _HiFrequency },

                    { "loNote", _LoNote },
                    { "hiNote", _HiNote },

                    { "bandsPerOctave", _BandsPerOctave },
                    { "tuningPitch", _TuningPitch },
                    { "transpose", _Transpose },

                    { "scalingFunction", _ScalingFunction },
                    { "skewFactor", _SkewFactor },
                    { "bandwidth", _Bandwidth },
                })
        ),

        // Acoustic Filters
        json::object_t::value_type
        (
            "acousticFilters", json::object
            ({
                { "weightingType", _WeightingType },

                { "slopeFunctionOffset", _SlopeFunctionOffset },
                { "slope", _Slope },
                { "slopeOffset", _SlopeOffset },

                { "equalizeAmount", _EqualizeAmount},
                { "equalizeOffset", _EqualizeOffset },
                { "equalizeDepth", _EqualizeDepth },

                { "weightingAmount", _WeightingAmount },
            })
        ), 

        // Common
        { "smoothingMethod", _SmoothingMethod },
        { "smoothingFactor", _SmoothingFactor },

        { "showToolTipsAlways", _ShowToolTipsAlways },
        { "suppressMirrorImage", _SuppressMirrorImage },
        { "visualizeDuringPause", _VisualizeDuringPause },

        json::object_t::value_type
        (
            "artwork", json::object
            ({
                { "type", _ArtworkType },

                { "colorCount", _NumArtworkColors },
                { "lightnessThreshold", _LightnessThreshold },
                { "transparencyThreshold", _TransparencyThreshold },

                { "colorOrder", _ColorOrder },

                { "showArtworkOnBackground", _ShowArtworkOnBackground },

                { "fitMode", _FitMode },
                { "fitWindow", _FitWindow },
                { "opacity", _ArtworkOpacity },
                { "filePath", msc::WideToUTF8(_ArtworkFilePath) },
            })
        ), 

        // Graphs
        json::object_t::value_type
        (
            "grid", json::object
            ({
                { "rows", _GridRowCount },
                { "columns", _GridColumnCount },
                { "verticalLayout", _VerticalLayout },
                { "graphs", json::array_t::value_type() },
            })
        ), 

        { "channelPair", _ChannelPair }, // FIXME: Shouldn't this be tied to a graph?

        // Styles
        { "styles", _StyleManager.ToJSON() },

        // Presets
        { "presetsDirectory", isPreset ? "" : msc::WideToUTF8(_PresetsDirectoryPath) },
    };

    auto & Graphs = Object["grid"]["graphs"];

    for (const auto & GraphDescription : _GraphDescriptions)
        Graphs.push_back(GraphDescription.ToJSON());

    return Object;
}

/// <summary>
/// One time conversion of the old color settings.
/// </summary>
void state_t::ConvertColorSettings() noexcept
{
    {
        style_t * style = _StyleManager.GetStyle(VisualElement::GraphBackground);

        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

        if ((_BackgroundMode_Deprecated > BackgroundMode::Artwork) && !_ArtworkGradientStops.empty())
        {
            _BackgroundMode_Deprecated = BackgroundMode::Artwork;

            style->_ColorSource = ColorSource::DominantColor;
            style->_CurrentColor = _StyleManager.DominantColor;
        }
        else
        if (!_UseCustomBackColor_Deprecated)
        {
            style->_ColorSource = ColorSource::UserInterface;
            style->_CurrentColor = _StyleManager.UserInterfaceColors[_IsDUI ? 1U : 3U];
        }
        else
        {
            style->_CustomColor = _BackColor_Deprecated;

            style->_ColorSource = ColorSource::Solid;
            style->_CurrentColor = style->_CustomColor;
        }

        if (_HorizontalGradient_Deprecated)
            style->_Flags |= style_t::Features::HorizontalGradient;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::VerticalGridLine);

        style->_CustomColor = _XLineColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomXLineColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager.UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::XAxisText);

        style->_CustomColor = _XTextColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomXTextColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager.UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::HorizontalGridLine);

        style->_CustomColor = _YLineColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomYLineColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager.UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::YAxisText);

        style->_CustomColor = _YTextColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomYTextColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager.UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::BarArea);

        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            style->_ColorSource = ColorSource::Gradient;
            style->_CurrentColor = D2D1::ColorF(0, 0.f);
            style->_CurrentGradientStops = SelectGradientStops_Deprecated(_ColorScheme_Deprecated);
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::BarDarkBackground);

        style->_CustomColor = _DarkBandColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

            style->_ColorSource = ColorSource::Solid;
            style->_CurrentColor = style->_CustomColor;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::BarLightBackground);

        style->_CustomColor = _LightBandColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

            style->_ColorSource = ColorSource::Solid;
            style->_CurrentColor = style->_CustomColor;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::CurveLine);

        style->_CustomColor = _LineColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomLineColor_Deprecated)
            {
                style->_ColorSource = ColorSource::Gradient;
                style->_CurrentColor = D2D1::ColorF(0, 0.f);
                style->_CurrentGradientStops = SelectGradientStops_Deprecated(_ColorScheme_Deprecated);
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_Thickness = _LineWidth_Deprecated;
    }

    {
        style_t * style = _StyleManager.GetStyle(VisualElement::CurveArea);

        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            style->_ColorSource = ColorSource::Gradient;
            style->_CurrentColor = D2D1::ColorF(0, 0.f);
            style->_CurrentGradientStops = SelectGradientStops_Deprecated(_ColorScheme_Deprecated);
            style->_Opacity = _AreaOpacity_Deprecated;
    }
}

/// <summary>
/// One time conversion of the old graph settings.
/// </summary>
void state_t::ConvertGraphDescription() noexcept
{
    for (auto & gs : _GraphDescriptions)
    {
        gs._XAxisMode     = _XAxisMode_Deprecated;
        gs._XAxisTop      = _XAxisTop_Deprecated;
        gs._XAxisBottom   = _XAxisBottom_Deprecated;

        gs._YAxisMode     = _YAxisMode_Deprecated;
        gs._YAxisLeft     = _YAxisLeft_Deprecated;
        gs._YAxisRight    = _YAxisRight_Deprecated;

        gs._AmplitudeLo   = _AmplitudeLo_Deprecated;
        gs._AmplitudeHi   = _AmplitudeHi_Deprecated;
        gs._AmplitudeStep = _AmplitudeStep_Deprecated;

        gs._UseAbsolute   = _UseAbsolute_Deprecated;
        gs._Gamma         = _Gamma_Deprecated;
    }
}

/// <summary>
/// Helper method to initialize the gradient stops vector during conversion.
/// </summary>
const gradient_stops_t state_t::SelectGradientStops_Deprecated(ColorScheme colorScheme) const noexcept
{
    if (colorScheme == ColorScheme::Custom)
        return _CustomGradientStops_Deprecated;

    if (colorScheme == ColorScheme::Artwork)
        return _ArtworkGradientStops;

    return GetBuiltInGradientStops(colorScheme);
}

cfg_int CfgLogLevel({ 0xf06c6211, 0x1617, 0x41ac, { 0xaf, 0xbe, 0x7f, 0xb3, 0xda, 0xef, 0x6, 0x69 } }, DefaultCfgLogLevel);

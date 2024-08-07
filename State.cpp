
/** $VER: State.cpp (2024.08.07) P. Stuer **/

#include "framework.h"
#include "State.h"

#include "Gradients.h"
#include "Log.h"

#include <SDK/file.h>

#include <pfc/string_conv.h>
#include <pfc/string-conv-lite.h>

using namespace pfc;
using namespace stringcvt;

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
State::State()
{
    Reset();
}

/// <summary>
/// Resets this instance.
/// </summary>
void State::Reset() noexcept
{
    _DialogBounds = { };
    _PageIndex = 0;

    _RefreshRateLimit = 20;

    _UseHardwareRendering = true;
    _UseAntialiasing = true;

    _UseZeroTrigger_Deprecated = false;
    _WindowDuration = 50;

    // Transform
    _Transform = Transform::FFT;

    _WindowFunction = WindowFunctions::Hann;
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

    // IIR
    _FilterBankOrder = 4;
    _TimeResolution = 600.;
    _IIRBandwidth = 1.;
    _ConstantQ = true;
    _CompensateBW = true;
    _PreWarpQ = false;

    // Brown-Puckette CQT-specific
    _BandwidthOffset = 1.;
    _BandwidthCap = 1.;
    _BandwidthAmount = 6.;
    _UseGranularBandwidth = true;

    _KernelShape = WindowFunctions::Nuttall;
    _KernelShapeParameter = 1.;
    _KernelAsymmetry = 0.;

    // Frequencies
    _FrequencyDistribution = FrequencyDistribution::Octaves;

    // Frequency range
    _BandCount = 320;
    _LoFrequency = 20.;
    _HiFrequency = 20000.;

    // Note range
    _MinNote = 0;
    _MaxNote = 126;
    _BandsPerOctave = 12;
    _Pitch = 440.0;
    _Transpose = 0;

    // Frequencies
    _ScalingFunction = ScalingFunction::Logarithmic;

    // Filters
    _WeightingType = WeightingType::None;

    _SlopeFunctionOffset = 1.;

    _Slope = 0.;
    _SlopeOffset = 1000.;

    _EqualizeAmount = 0.;
    _EqualizeOffset = 44100.;
    _EqualizeDepth = 1024.;

    _WeightingAmount = 0.;

    _SkewFactor = 0.0;
    _Bandwidth = 0.5;

    _SmoothingMethod = SmoothingMethod::Average;
    _SmoothingFactor = 0.5;

    _KernelSize = 32;
    _SummationMethod = SummationMethod::Maximum;
    _SmoothLowerFrequencies = true;
    _SmoothGainTransition = true;

    // Rendering parameters
    _BackColor_Deprecated = D2D1::ColorF(0.f, 0.f, 0.f, 1.f);                  // Deprecated
    _UseCustomBackColor_Deprecated = true;                                     // Deprecated

    // X axis
    _XAxisMode_Deprecated = XAxisMode::Notes;
    _XAxisTop_Deprecated = true;
    _XAxisBottom_Deprecated = true;

    _XTextColor_Deprecated = D2D1::ColorF(D2D1::ColorF::White);                // Deprecated
    _UseCustomXTextColor_Deprecated = true;                                    // Deprecated

    _XLineColor_Deprecated = D2D1::ColorF(.25f, .25f, .25f, 1.f);              // Deprecated
    _UseCustomXLineColor_Deprecated = true;                                    // Deprecated

    // Y axis
    _YAxisMode_Deprecated = YAxisMode::Decibels;
    _YAxisLeft_Deprecated = true;
    _YAxisRight_Deprecated = true;

    _YTextColor_Deprecated = D2D1::ColorF(D2D1::ColorF::White);                // Deprecated
    _UseCustomYTextColor_Deprecated = true;                                    // Deprecated

    _YLineColor_Deprecated = D2D1::ColorF(.25f, .25f, .25f, 1.f);              // Deprecated
    _UseCustomYLineColor_Deprecated = true;                                    // Deprecated

    _AmplitudeLo_Deprecated = -90.;
    _AmplitudeHi_Deprecated =   0.;
    _AmplitudeStep_Deprecated = -6.;

    _UseAbsolute_Deprecated = true;
    _Gamma_Deprecated = 1.;

    // Common
    _ColorScheme_Deprecated = ColorScheme::Prism1;

    _GradientStops = GetGradientStops(_ColorScheme_Deprecated);
    _CustomGradientStops_Deprecated = GetGradientStops(ColorScheme::Custom);

    _ShowToolTips = true;
    _SuppressMirrorImage = true;

    // Artwork
    _NumArtworkColors = 10;
    _LightnessThreshold = 250.f / 255.f;
    _TransparencyThreshold = 125.f / 255.f;

    _ColorOrder = ColorOrder::None;

    _BackgroundMode_Deprecated = BackgroundMode::Artwork;
    _ShowArtworkOnBackground = true;
    _ArtworkOpacity = 1.f;
    _ArtworkFilePath.clear();
    _FitMode = FitMode::FitBig;
    _FitWindow = false;

    /** Graphs **/

    _GraphSettings.clear();

    _GraphSettings.push_back(GraphSettings(L"Stereo"));

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
    _LEDSize = 2.f;
    _LEDGap = 2.f;
    _HorizontalGradient_Deprecated = false;

    _PeakMode = PeakMode::Classic;
    _HoldTime = 30.;
    _Acceleration = 0.5;

    // Curve
    _LineWidth_Deprecated = 2.f;
    _LineColor_Deprecated = D2D1::ColorF(D2D1::ColorF::White);
    _UseCustomLineColor_Deprecated = false;
    _PeakLineColor_Deprecated = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _UseCustomPeakLineColor_Deprecated = false;
    _AreaOpacity_Deprecated = 0.5f;

    // Spectogram
    _ScrollingSpectogram = true;
    _HorizontalSpectogram = true;
    _UseSpectrumBarMetrics = false;

    // Peak Meter
    _HorizontalPeakMeter = false;
    _RMSPlus3 = false;
    _RMSWindow = .300; // seconds
    _GaugeGap = 1.f; // pixels

    _ChannelPair = ChannelPair::FrontLeftRight;
    _HorizontalLevelMeter = false;

    _StyleManager.Reset();

    pfc::string Path = core_api::get_profile_path();

    if (Path.startsWith("file://"))
        _PresetsDirectoryPath = ::wideFromUTF8(Path + strlen("file://"));
    else
        _PresetsDirectoryPath = ::wideFromUTF8(Path);

    /* Not serialized */

    _UseToneGenerator = false;

    _SampleRate = 0;

    _Barrier = 0;
    _ActivePresetName.clear();
}

/// <summary>
/// Implements the = operator.
/// </summary>
State & State::operator=(const State & other)
{
    _DialogBounds = other._DialogBounds;
    _PageIndex = other._PageIndex;

    _RefreshRateLimit = other._RefreshRateLimit;

    _UseHardwareRendering = other._UseHardwareRendering;
    _UseAntialiasing = other._UseAntialiasing;

    _UseZeroTrigger_Deprecated = other._UseZeroTrigger_Deprecated;
    _WindowDuration = other._WindowDuration;

    #pragma region Transform

        _Transform = other._Transform;

        _WindowFunction = other._WindowFunction;
        _WindowParameter = other._WindowParameter;
        _WindowSkew = other._WindowSkew;
        _Truncate = other._Truncate;

        _ReactionAlignment = other._ReactionAlignment;

        _Channels_Deprecated = other._Channels_Deprecated;

    #pragma endregion

    #pragma region FFT

        _FFTMode = other._FFTMode;
        _FFTCustom = other._FFTCustom;
        _FFTDuration = other._FFTDuration;

        _KernelSize = other._KernelSize;
        _SummationMethod = other._SummationMethod;
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
        _CompensateBW = other._CompensateBW;
        _PreWarpQ = other._PreWarpQ;

    #pragma endregion

    #pragma region Frequencies

        _FrequencyDistribution = other._FrequencyDistribution;

        _BandCount = other._BandCount;
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

        _BackColor_Deprecated = other._BackColor_Deprecated;
        _UseCustomBackColor_Deprecated = other._UseCustomBackColor_Deprecated;

        // X axis
        _XAxisMode_Deprecated = other._XAxisMode_Deprecated;
        _XAxisTop_Deprecated = other._XAxisTop_Deprecated;
        _XAxisBottom_Deprecated = other._XAxisBottom_Deprecated;

        _XTextColor_Deprecated = other._XTextColor_Deprecated;
        _UseCustomXTextColor_Deprecated = other._UseCustomXTextColor_Deprecated;

        _XLineColor_Deprecated = other._XLineColor_Deprecated;
        _UseCustomXLineColor_Deprecated = other._UseCustomXLineColor_Deprecated;

        // Y axis
        _YAxisMode_Deprecated = other._YAxisMode_Deprecated;
        _YAxisLeft_Deprecated = other._YAxisLeft_Deprecated;
        _YAxisRight_Deprecated = other._YAxisRight_Deprecated;

        _YTextColor_Deprecated = other._YTextColor_Deprecated;
        _UseCustomYTextColor_Deprecated = other._UseCustomYTextColor_Deprecated;

        _YLineColor_Deprecated = other._YLineColor_Deprecated;
        _UseCustomYLineColor_Deprecated = other._UseCustomYLineColor_Deprecated;

        _AmplitudeLo_Deprecated = other._AmplitudeLo_Deprecated;
        _AmplitudeHi_Deprecated = other._AmplitudeHi_Deprecated;
        _AmplitudeStep_Deprecated = other._AmplitudeStep_Deprecated;

        _UseAbsolute_Deprecated = other._UseAbsolute_Deprecated;

        _Gamma_Deprecated = other._Gamma_Deprecated;

    #pragma endregion

    #pragma region Graph Common

        // Common
        _ColorScheme_Deprecated = other._ColorScheme_Deprecated;                      // Deprecated

        _SmoothingMethod = other._SmoothingMethod;
        _SmoothingFactor = other._SmoothingFactor;

        _GradientStops = other._GradientStops;                  // Deprecated
        _CustomGradientStops_Deprecated = other._CustomGradientStops_Deprecated;      // Deprecated

        _ShowToolTips = other._ShowToolTips;
        _SuppressMirrorImage = other._SuppressMirrorImage;

        // Artwork
        _NumArtworkColors = other._NumArtworkColors;
        _LightnessThreshold = other._LightnessThreshold;
        _TransparencyThreshold = other._TransparencyThreshold;

        _ColorOrder = other._ColorOrder;

        _BackgroundMode_Deprecated = other._BackgroundMode_Deprecated;                //Deprecated
        _ShowArtworkOnBackground = other._ShowArtworkOnBackground;
        _ArtworkOpacity = other._ArtworkOpacity;
        _ArtworkFilePath = other._ArtworkFilePath;
        _FitMode = other._FitMode;
        _FitWindow = other._FitWindow;

    #pragma endregion

    #pragma region Graphs

        _GraphSettings = other._GraphSettings;

        _VerticalLayout = other._VerticalLayout;

        _GridRowCount = other._GridRowCount;
        _GridColumnCount = other._GridColumnCount;

    #pragma endregion

    #pragma region Visualization

    _VisualizationType = other._VisualizationType;

    // Bars
    _DrawBandBackground_Deprecated = other._DrawBandBackground_Deprecated;
    _LightBandColor_Deprecated = other._LightBandColor_Deprecated;
    _DarkBandColor_Deprecated = other._DarkBandColor_Deprecated;
    _LEDMode = other._LEDMode;
    _LEDSize = other._LEDSize;
    _LEDGap = other._LEDGap;
    _HorizontalGradient_Deprecated = other._HorizontalGradient_Deprecated;

    _PeakMode = other._PeakMode;
    _HoldTime = other._HoldTime;
    _Acceleration = other._Acceleration;

    // Curve
    _LineWidth_Deprecated = other._LineWidth_Deprecated;
    _LineColor_Deprecated = other._LineColor_Deprecated;
    _UseCustomLineColor_Deprecated = other._UseCustomLineColor_Deprecated;
    _PeakLineColor_Deprecated = other._PeakLineColor_Deprecated;
    _UseCustomPeakLineColor_Deprecated = other._UseCustomPeakLineColor_Deprecated;
    _AreaOpacity_Deprecated = other._AreaOpacity_Deprecated;

    // Spectogram
    _ScrollingSpectogram = other._ScrollingSpectogram;
    _HorizontalSpectogram = other._HorizontalSpectogram;
    _UseSpectrumBarMetrics = other._UseSpectrumBarMetrics;

    // Peak Meter
    _HorizontalPeakMeter = other._HorizontalPeakMeter;
    _RMSPlus3 = other._RMSPlus3;
    _RMSWindow = other._RMSWindow;
    _GaugeGap = other._GaugeGap;

    // Level Meter
    _ChannelPair = other._ChannelPair;
    _HorizontalLevelMeter = other._HorizontalLevelMeter;

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
/// Reads this instance with the specified reader. (CUI version)
/// </summary>
void State::Read(stream_reader * reader, size_t size, abort_callback & abortHandler, bool isPreset) noexcept
{
    Reset();

    size_t Version;

    if (size < sizeof(Version))
        return;

    try
    {
        reader->read(&Version, sizeof(Version), abortHandler);

        if (Version > _CurrentVersion)
            return;

        reader->read(&_DialogBounds, sizeof(_DialogBounds), abortHandler);

        reader->read(&_RefreshRateLimit, sizeof(_RefreshRateLimit), abortHandler); _RefreshRateLimit = std::clamp<size_t>(_RefreshRateLimit, 20, 200);

        reader->read(&_UseHardwareRendering, sizeof(_UseHardwareRendering), abortHandler);
        reader->read(&_UseAntialiasing, sizeof(_UseAntialiasing), abortHandler);

        reader->read(&_UseZeroTrigger_Deprecated, sizeof(_UseZeroTrigger_Deprecated), abortHandler);

        reader->read(&_WindowDuration, sizeof(_WindowDuration), abortHandler); _WindowDuration = std::clamp<size_t>(_WindowDuration, 50, 800);

        reader->read(&_Transform, sizeof(_Transform), abortHandler);

    #pragma region FFT

        reader->read(&_FFTMode, sizeof(_FFTMode), abortHandler);

        // v17: FFTMode::65536 was added before FFTMode::Custom
        if (Version < 17)
            _FFTMode = (FFTMode) ((int) _FFTMode + 1);

        reader->read(&_FFTCustom, sizeof(_FFTCustom), abortHandler);
        reader->read(&_FFTDuration, sizeof(_FFTDuration), abortHandler);

        reader->read(&_MappingMethod, sizeof(_MappingMethod), abortHandler);
        reader->read(&_SmoothingMethod, sizeof(_SmoothingMethod), abortHandler);

        // v15: SmoothingMethod::None was inserted before SmoothingMethod::Average.
        if (Version < 15)
            _SmoothingMethod = (SmoothingMethod) ((int) _SmoothingMethod + 1);

        reader->read(&_SmoothingFactor, sizeof(_SmoothingFactor), abortHandler);
        reader->read(&_KernelSize, sizeof(_KernelSize), abortHandler);
        reader->read(&_SummationMethod, sizeof(_SummationMethod), abortHandler);
        reader->read(&_SmoothLowerFrequencies, sizeof(_SmoothLowerFrequencies), abortHandler);
        reader->read(&_SmoothGainTransition, sizeof(_SmoothGainTransition), abortHandler);

    #pragma endregion

    #pragma region Frequencies
        reader->read(&_FrequencyDistribution, sizeof(_FrequencyDistribution), abortHandler);

        reader->read(&_BandCount, sizeof(_BandCount), abortHandler);

        reader->read(&_LoFrequency, sizeof(_LoFrequency), abortHandler);
        reader->read(&_HiFrequency, sizeof(_HiFrequency), abortHandler);

        reader->read(&_MinNote, sizeof(_MinNote), abortHandler);
        reader->read(&_MaxNote, sizeof(_MaxNote), abortHandler);
        reader->read(&_BandsPerOctave, sizeof(_BandsPerOctave), abortHandler);
        reader->read(&_Pitch, sizeof(_Pitch), abortHandler);
        reader->read(&_Transpose, sizeof(_Transpose), abortHandler);

        reader->read(&_ScalingFunction, sizeof(_ScalingFunction), abortHandler);
        reader->read(&_SkewFactor, sizeof(_SkewFactor), abortHandler);
        reader->read(&_Bandwidth, sizeof(_Bandwidth), abortHandler);
    #pragma endregion

    #pragma region Rendering

        reader->read(&_BackColor_Deprecated, sizeof(_BackColor_Deprecated), abortHandler);

        reader->read(&_XAxisMode_Deprecated, sizeof(_XAxisMode_Deprecated), abortHandler);

        reader->read(&_YAxisMode_Deprecated, sizeof(_YAxisMode_Deprecated), abortHandler);

        reader->read(&_AmplitudeLo_Deprecated, sizeof(_AmplitudeLo_Deprecated), abortHandler);
        reader->read(&_AmplitudeHi_Deprecated, sizeof(_AmplitudeHi_Deprecated), abortHandler);
        reader->read(&_UseAbsolute_Deprecated, sizeof(_UseAbsolute_Deprecated), abortHandler);
        reader->read(&_Gamma_Deprecated, sizeof(_Gamma_Deprecated), abortHandler);

        reader->read(&_ColorScheme_Deprecated, sizeof(_ColorScheme_Deprecated), abortHandler);

        if ((Version <= 9) && (_ColorScheme_Deprecated != ColorScheme::Solid) && (_ColorScheme_Deprecated != ColorScheme::Custom))
            _ColorScheme_Deprecated = (ColorScheme) ((int) _ColorScheme_Deprecated + 1); // ColorScheme::Artwork was added after ColorScheme::Custom

        reader->read(&_DrawBandBackground_Deprecated, sizeof(_DrawBandBackground_Deprecated), abortHandler);

        reader->read(&_PeakMode, sizeof(_PeakMode), abortHandler);
        reader->read(&_HoldTime, sizeof(_HoldTime), abortHandler);
        reader->read(&_Acceleration, sizeof(_Acceleration), abortHandler);

    #pragma endregion

        _CustomGradientStops_Deprecated.clear();

        size_t Count; reader->read(&Count, sizeof(Count), abortHandler);

        for (size_t i = 0; i < Count; ++i)
        {
            D2D1_GRADIENT_STOP gs = { };

            reader->read(&gs.position, sizeof(gs.position), abortHandler);
            reader->read(&gs.color, sizeof(gs.color), abortHandler);

            _CustomGradientStops_Deprecated.push_back(gs);
        }

        reader->read(&_XTextColor_Deprecated, sizeof(_XTextColor_Deprecated), abortHandler);
        reader->read(&_XLineColor_Deprecated, sizeof(_XLineColor_Deprecated), abortHandler);
        reader->read(&_YTextColor_Deprecated, sizeof(_YTextColor_Deprecated), abortHandler);
        reader->read(&_YLineColor_Deprecated, sizeof(_YLineColor_Deprecated), abortHandler);
        reader->read(&_DarkBandColor_Deprecated, sizeof(_DarkBandColor_Deprecated), abortHandler);

        reader->read(&_AmplitudeStep_Deprecated, sizeof(_AmplitudeStep_Deprecated), abortHandler);

        reader->read(&_Channels_Deprecated, sizeof(_Channels_Deprecated), abortHandler);
        reader->read(&_ShowToolTips, sizeof(_ShowToolTips), abortHandler);

        reader->read(&_WindowFunction, sizeof(_WindowFunction), abortHandler);
        reader->read(&_WindowParameter, sizeof(_WindowParameter), abortHandler);
        reader->read(&_WindowSkew, sizeof(_WindowSkew), abortHandler);

        if (Version >= 8)
        {
            reader->read(&_UseCustomBackColor_Deprecated, sizeof(_UseCustomBackColor_Deprecated), abortHandler);
            reader->read(&_UseCustomXTextColor_Deprecated, sizeof(_UseCustomXTextColor_Deprecated), abortHandler);
            reader->read(&_UseCustomXLineColor_Deprecated, sizeof(_UseCustomXLineColor_Deprecated), abortHandler);
            reader->read(&_UseCustomYTextColor_Deprecated, sizeof(_UseCustomYTextColor_Deprecated), abortHandler);
            reader->read(&_UseCustomYLineColor_Deprecated, sizeof(_UseCustomYLineColor_Deprecated), abortHandler);

            reader->read(&_LEDMode, sizeof(_LEDMode), abortHandler);

            reader->read(&_HorizontalGradient_Deprecated, sizeof(_HorizontalGradient_Deprecated), abortHandler);

            reader->read(&_LightBandColor_Deprecated, sizeof(_LightBandColor_Deprecated), abortHandler);
        }

        if (Version >= 9)
        {
            reader->read(&_PageIndex, sizeof(_PageIndex), abortHandler);
            reader->read(&_VisualizationType, sizeof(_VisualizationType), abortHandler);
            reader->read(&_LineWidth_Deprecated, sizeof(_LineWidth_Deprecated), abortHandler);
            reader->read(&_AreaOpacity_Deprecated, sizeof(_AreaOpacity_Deprecated), abortHandler);
        }

        if (Version >= 10)
        {
            reader->read(&_BackgroundMode_Deprecated, sizeof(_BackgroundMode_Deprecated), abortHandler); _BackgroundMode_Deprecated = std::clamp(_BackgroundMode_Deprecated, BackgroundMode::None, BackgroundMode::Artwork);

            _ShowArtworkOnBackground = (_BackgroundMode_Deprecated == BackgroundMode::Artwork);

            reader->read(&_ArtworkOpacity, sizeof(_ArtworkOpacity), abortHandler);

            reader->read(&_NumArtworkColors, sizeof(_NumArtworkColors), abortHandler);
            reader->read(&_LightnessThreshold, sizeof(_LightnessThreshold), abortHandler);
            reader->read(&_ColorOrder, sizeof(_ColorOrder), abortHandler);
        }

        if (Version >= 11)
        {
            reader->read(&_WeightingType, sizeof(_WeightingType), abortHandler);

            reader->read(&_SlopeFunctionOffset, sizeof(_SlopeFunctionOffset), abortHandler);

            reader->read(&_Slope, sizeof(_Slope), abortHandler);
            reader->read(&_SlopeOffset, sizeof(_SlopeOffset), abortHandler);

            reader->read(&_EqualizeAmount, sizeof(_EqualizeAmount), abortHandler);
            reader->read(&_EqualizeOffset, sizeof(_EqualizeOffset), abortHandler);
            reader->read(&_EqualizeDepth, sizeof(_EqualizeDepth), abortHandler);

            reader->read(&_WeightingAmount, sizeof(_WeightingAmount), abortHandler);

            reader->read(&_LineColor_Deprecated, sizeof(_LineColor_Deprecated), abortHandler);
            reader->read(&_UseCustomLineColor_Deprecated, sizeof(_UseCustomLineColor_Deprecated), abortHandler);
            reader->read(&_PeakLineColor_Deprecated, sizeof(_PeakLineColor_Deprecated), abortHandler);
            reader->read(&_UseCustomPeakLineColor_Deprecated, sizeof(_UseCustomPeakLineColor_Deprecated), abortHandler);
        }

        if (Version >= 12)
        {
            reader->read(&_BandwidthOffset, sizeof(_BandwidthOffset), abortHandler);
            reader->read(&_BandwidthCap, sizeof(_BandwidthCap), abortHandler);
            reader->read(&_BandwidthAmount, sizeof(_BandwidthAmount), abortHandler);
            reader->read(&_UseGranularBandwidth, sizeof(_UseGranularBandwidth), abortHandler);

            reader->read(&_KernelShape, sizeof(_KernelShape), abortHandler);
            reader->read(&_KernelShapeParameter, sizeof(_KernelShapeParameter), abortHandler);
            reader->read(&_KernelAsymmetry, sizeof(_KernelAsymmetry), abortHandler);
        }

        if (Version <= 13)
            ConvertColorSettings();

        if (Version >= 13)
        {
            pfc::string Path = reader->read_string(abortHandler);

            _ArtworkFilePath = pfc::wideFromUTF8(Path);
        }

        if (Version >= 14)
            _StyleManager.Read(reader, size, abortHandler);

        if (Version >= 16)
        {
            reader->read_object_t(_ReactionAlignment, abortHandler);

            reader->read_object_t(_XAxisTop_Deprecated, abortHandler);
            reader->read_object_t(_XAxisBottom_Deprecated, abortHandler);
            reader->read_object_t(_YAxisLeft_Deprecated, abortHandler);
            reader->read_object_t(_YAxisRight_Deprecated, abortHandler);

            reader->read_object_t(_FilterBankOrder, abortHandler);
            reader->read_object_t(_TimeResolution, abortHandler);
            reader->read_object_t(_IIRBandwidth, abortHandler);

            reader->read_object_t(_SuppressMirrorImage, abortHandler);
        }

        if (Version <= 17)
            ConvertGraphSettings();

        if (Version >= 17)
        {
            reader->read(&_FitMode, sizeof(_FitMode), abortHandler);
        }

        // Version 18, v0.7.1.0-beta-2
        if (Version >= 18)
        {
            reader->read_object_t(_ShowArtworkOnBackground, abortHandler);

            reader->read_object_t(_GridRowCount, abortHandler);
            reader->read_object_t(_GridColumnCount, abortHandler);

            uint32_t GraphSettingsVersion;

            reader->read_object_t(GraphSettingsVersion, abortHandler);

            reader->read_object_t(_VerticalLayout, abortHandler);

            _GraphSettings.clear();

            reader->read_object_t(Count, abortHandler);

            for (size_t i = 0; i < Count; ++i)
            {
                GraphSettings gs;

                pfc::string Description; reader->read_string(Description, abortHandler); gs._Description = pfc::wideFromUTF8(Description);
                reader->read_object_t(gs._Channels, abortHandler);
                reader->read_object_t(gs._FlipHorizontally, abortHandler);
                reader->read_object_t(gs._FlipVertically, abortHandler);

                reader->read_object(&gs._XAxisMode, sizeof(gs._XAxisMode), abortHandler);
                reader->read_object_t(gs._XAxisTop, abortHandler);
                reader->read_object_t(gs._XAxisBottom, abortHandler);

                reader->read_object(&gs._YAxisMode, sizeof(gs._YAxisMode), abortHandler);
                reader->read_object_t(gs._YAxisLeft, abortHandler);
                reader->read_object_t(gs._YAxisRight, abortHandler);

                reader->read_object_t(gs._AmplitudeLo, abortHandler);
                reader->read_object_t(gs._AmplitudeHi, abortHandler);
                reader->read_object_t(gs._AmplitudeStep, abortHandler);

                reader->read_object_t(gs._UseAbsolute, abortHandler);
                reader->read_object_t(gs._Gamma, abortHandler);

                reader->read_object_t(gs._HRatio, abortHandler);
                reader->read_object_t(gs._VRatio, abortHandler);

                if (GraphSettingsVersion > 1)
                {
                    reader->read_object_t(gs._LPadding, abortHandler);
                    reader->read_object_t(gs._RPadding, abortHandler);
                    reader->read_object_t(gs._TPadding, abortHandler);
                    reader->read_object_t(gs._BPadding, abortHandler);

                    reader->read_object(&gs._HAlignment, sizeof(gs._HAlignment), abortHandler);
                    reader->read_object(&gs._VAlignment, sizeof(gs._VAlignment), abortHandler);
                }

                _GraphSettings.push_back(gs);
            }
        }

        // Version 19, v0.7.2.0
        if (Version >= 19)
        {
            reader->read_object_t(_ConstantQ, abortHandler);
            reader->read_object_t(_CompensateBW, abortHandler);
            reader->read_object_t(_PreWarpQ, abortHandler);
        }

        if (Version >= 20)
        {
            pfc::string Path;

            reader->read_string(Path, abortHandler); 

            if (!isPreset)
                _PresetsDirectoryPath = pfc::wideFromUTF8(Path);
        }

        if (Version >= 21)
        {
            reader->read_object_t(_ScrollingSpectogram, abortHandler);
        }

        if (Version >= 22)
        {
            reader->read_object_t(_HorizontalPeakMeter, abortHandler);
        }

        if (Version >= 23)
        {
            reader->read_object_t(_LEDSize, abortHandler);
            reader->read_object_t(_LEDGap, abortHandler);
        }

        if (Version >= 24)
        {
            reader->read_object_t(_FitWindow, abortHandler);
        }

        if (Version >= 25)
        {
            reader->read_object_t(_RMSWindow, abortHandler);
        }

        if (Version >= 26)
        {
            reader->read_object_t(_GaugeGap, abortHandler);
            reader->read_object_t(_RMSPlus3, abortHandler);
        }

        if (Version >= 27)
        {
            reader->read(&_ChannelPair, sizeof(_ChannelPair), abortHandler);
            _ChannelPair = std::clamp(_ChannelPair, ChannelPair::FrontLeftRight, ChannelPair::TopBackLeftRight);

            reader->read_object_t(_HorizontalLevelMeter, abortHandler);
            reader->read_object_t(_HorizontalSpectogram, abortHandler);
            reader->read_object_t(_UseSpectrumBarMetrics, abortHandler);
        }
    }
    catch (exception & ex)
    {
        Log::Write(Log::Level::Error, "%8d: %s failed to read DUI configuration: %s", (uint32_t) ::GetTickCount64(), core_api::get_my_file_name(), ex.what());

        Reset();
    }
}

/// <summary>
/// Writes this instance to the specified writer. (CUI version)
/// </summary>
void State::Write(stream_writer * writer, abort_callback & abortHandler, bool isPreset) const noexcept
{
    try
    {
        writer->write(&_CurrentVersion, sizeof(_CurrentVersion), abortHandler);

        #pragma region User Interface

        writer->write(&_DialogBounds, sizeof(_DialogBounds), abortHandler);

        writer->write(&_RefreshRateLimit, sizeof(_RefreshRateLimit), abortHandler);

        writer->write(&_UseHardwareRendering, sizeof(_UseHardwareRendering), abortHandler);
        writer->write(&_UseAntialiasing, sizeof(_UseAntialiasing), abortHandler);

        writer->write(&_UseZeroTrigger_Deprecated, sizeof(_UseZeroTrigger_Deprecated), abortHandler);
        writer->write(&_WindowDuration, sizeof(_WindowDuration), abortHandler);

        #pragma endregion

        writer->write(&_Transform, sizeof(_Transform), abortHandler);

        #pragma region FFT

        writer->write(&_FFTMode, sizeof(_FFTMode), abortHandler);
        writer->write(&_FFTCustom, sizeof(_FFTCustom), abortHandler);
        writer->write(&_FFTDuration, sizeof(_FFTDuration), abortHandler);
        writer->write(&_MappingMethod, sizeof(_MappingMethod), abortHandler);

        writer->write(&_SmoothingMethod, sizeof(_SmoothingMethod), abortHandler);
        writer->write(&_SmoothingFactor, sizeof(_SmoothingFactor), abortHandler);
        writer->write(&_KernelSize, sizeof(_KernelSize), abortHandler);
        writer->write(&_SummationMethod, sizeof(_SummationMethod), abortHandler);
        writer->write(&_SmoothLowerFrequencies, sizeof(_SmoothLowerFrequencies), abortHandler);
        writer->write(&_SmoothGainTransition, sizeof(_SmoothGainTransition), abortHandler);

        #pragma endregion

        #pragma region Frequencies

        writer->write(&_FrequencyDistribution, sizeof(_FrequencyDistribution), abortHandler);

        writer->write(&_BandCount, sizeof(_BandCount), abortHandler);
        writer->write(&_LoFrequency, sizeof(_LoFrequency), abortHandler);
        writer->write(&_HiFrequency, sizeof(_HiFrequency), abortHandler);

        writer->write(&_MinNote, sizeof(_MinNote), abortHandler);
        writer->write(&_MaxNote, sizeof(_MaxNote), abortHandler);
        writer->write(&_BandsPerOctave, sizeof(_BandsPerOctave), abortHandler);
        writer->write(&_Pitch, sizeof(_Pitch), abortHandler);
        writer->write(&_Transpose, sizeof(_Transpose), abortHandler);

        writer->write(&_ScalingFunction, sizeof(_ScalingFunction), abortHandler);
        writer->write(&_SkewFactor, sizeof(_SkewFactor), abortHandler);
        writer->write(&_Bandwidth, sizeof(_Bandwidth), abortHandler);

        #pragma endregion

        #pragma region Rendering

        writer->write(&_BackColor_Deprecated, sizeof(_BackColor_Deprecated), abortHandler);

        writer->write(&_XAxisMode_Deprecated, sizeof(_XAxisMode_Deprecated), abortHandler);

        writer->write(&_YAxisMode_Deprecated, sizeof(_YAxisMode_Deprecated), abortHandler);

        writer->write(&_AmplitudeLo_Deprecated, sizeof(_AmplitudeLo_Deprecated), abortHandler);
        writer->write(&_AmplitudeHi_Deprecated, sizeof(_AmplitudeHi_Deprecated), abortHandler);
        writer->write(&_UseAbsolute_Deprecated, sizeof(_UseAbsolute_Deprecated), abortHandler);
        writer->write(&_Gamma_Deprecated, sizeof(_Gamma_Deprecated), abortHandler);

        writer->write(&_ColorScheme_Deprecated, sizeof(_ColorScheme_Deprecated), abortHandler);

        writer->write(&_DrawBandBackground_Deprecated, sizeof(_DrawBandBackground_Deprecated), abortHandler);

        writer->write(&_PeakMode, sizeof(_PeakMode), abortHandler);
        writer->write(&_HoldTime, sizeof(_HoldTime), abortHandler);
        writer->write(&_Acceleration, sizeof(_Acceleration), abortHandler);

        #pragma endregion

        size_t Size = _CustomGradientStops_Deprecated.size();

        writer->write(&Size, sizeof(Size), abortHandler);

        for (const auto & Iter : _CustomGradientStops_Deprecated)
        {
            writer->write(&Iter.position, sizeof(Iter.position), abortHandler);
            writer->write(&Iter.color, sizeof(Iter.color), abortHandler);
        }

        writer->write(&_XTextColor_Deprecated, sizeof(_XTextColor_Deprecated), abortHandler);
        writer->write(&_XLineColor_Deprecated, sizeof(_XLineColor_Deprecated), abortHandler);
        writer->write(&_YTextColor_Deprecated, sizeof(_YTextColor_Deprecated), abortHandler);
        writer->write(&_YLineColor_Deprecated, sizeof(_YLineColor_Deprecated), abortHandler);
        writer->write(&_DarkBandColor_Deprecated, sizeof(_DarkBandColor_Deprecated), abortHandler);

        writer->write(&_AmplitudeStep_Deprecated, sizeof(_AmplitudeStep_Deprecated), abortHandler);

        writer->write(&_Channels_Deprecated, sizeof(_Channels_Deprecated), abortHandler);
        writer->write(&_ShowToolTips, sizeof(_ShowToolTips), abortHandler);

        writer->write(&_WindowFunction, sizeof(_WindowFunction), abortHandler);
        writer->write(&_WindowParameter, sizeof(_WindowParameter), abortHandler);
        writer->write(&_WindowSkew, sizeof(_WindowSkew), abortHandler);

        // Version 8
        writer->write(&_UseCustomBackColor_Deprecated,  sizeof(_UseCustomBackColor_Deprecated), abortHandler);
        writer->write(&_UseCustomXTextColor_Deprecated, sizeof(_UseCustomXTextColor_Deprecated), abortHandler);
        writer->write(&_UseCustomXLineColor_Deprecated, sizeof(_UseCustomXLineColor_Deprecated), abortHandler);
        writer->write(&_UseCustomYTextColor_Deprecated, sizeof(_UseCustomYTextColor_Deprecated), abortHandler);
        writer->write(&_UseCustomYLineColor_Deprecated, sizeof(_UseCustomYLineColor_Deprecated), abortHandler);

        writer->write(&_LEDMode, sizeof(_LEDMode), abortHandler);

        writer->write(&_HorizontalGradient_Deprecated, sizeof(_HorizontalGradient_Deprecated), abortHandler);

        writer->write(&_LightBandColor_Deprecated, sizeof(_LightBandColor_Deprecated), abortHandler);

        // Version 9
        writer->write(&_PageIndex, sizeof(_PageIndex), abortHandler);
        writer->write(&_VisualizationType, sizeof(_VisualizationType), abortHandler);
        writer->write(&_LineWidth_Deprecated, sizeof(_LineWidth_Deprecated), abortHandler);
        writer->write(&_AreaOpacity_Deprecated, sizeof(_AreaOpacity_Deprecated), abortHandler);

        // Version 10
        writer->write(&_BackgroundMode_Deprecated, sizeof(_BackgroundMode_Deprecated), abortHandler);
        writer->write(&_ArtworkOpacity, sizeof(_ArtworkOpacity), abortHandler);

        writer->write(&_NumArtworkColors, sizeof(_NumArtworkColors), abortHandler);
        writer->write(&_LightnessThreshold, sizeof(_LightnessThreshold), abortHandler);
        writer->write(&_ColorOrder, sizeof(_ColorOrder), abortHandler);

        // Version 11
        writer->write(&_WeightingType, sizeof(_WeightingType), abortHandler);

        writer->write(&_SlopeFunctionOffset, sizeof(_SlopeFunctionOffset), abortHandler);

        writer->write(&_Slope, sizeof(_Slope), abortHandler);
        writer->write(&_SlopeOffset, sizeof(_SlopeOffset), abortHandler);

        writer->write(&_EqualizeAmount, sizeof(_EqualizeAmount), abortHandler);
        writer->write(&_EqualizeOffset, sizeof(_EqualizeOffset), abortHandler);
        writer->write(&_EqualizeDepth, sizeof(_EqualizeDepth), abortHandler);

        writer->write(&_WeightingAmount, sizeof(_WeightingAmount), abortHandler);

        writer->write(&_LineColor_Deprecated, sizeof(_LineColor_Deprecated), abortHandler);
        writer->write(&_UseCustomLineColor_Deprecated, sizeof(_UseCustomLineColor_Deprecated), abortHandler);
        writer->write(&_PeakLineColor_Deprecated, sizeof(_PeakLineColor_Deprecated), abortHandler);
        writer->write(&_UseCustomPeakLineColor_Deprecated, sizeof(_UseCustomPeakLineColor_Deprecated), abortHandler);

        // Version 12
        writer->write(&_BandwidthOffset, sizeof(_BandwidthOffset), abortHandler);
        writer->write(&_BandwidthCap, sizeof(_BandwidthCap), abortHandler);
        writer->write(&_BandwidthAmount, sizeof(_BandwidthAmount), abortHandler);
        writer->write(&_UseGranularBandwidth, sizeof(_UseGranularBandwidth), abortHandler);

        writer->write(&_KernelShape, sizeof(_KernelShape), abortHandler);
        writer->write(&_KernelShapeParameter, sizeof(_KernelShapeParameter), abortHandler);
        writer->write(&_KernelAsymmetry, sizeof(_KernelAsymmetry), abortHandler);

        // Version 13
        {
            pfc::string Path = pfc::utf8FromWide(_ArtworkFilePath.c_str());

            writer->write_string(Path, abortHandler);
        }

        // Version 15
        _StyleManager.Write(writer, abortHandler);

        // Version 16
        writer->write_object_t(_ReactionAlignment, abortHandler);

        writer->write_object_t(_XAxisTop_Deprecated, abortHandler);
        writer->write_object_t(_XAxisBottom_Deprecated, abortHandler);
        writer->write_object_t(_YAxisLeft_Deprecated, abortHandler);
        writer->write_object_t(_YAxisRight_Deprecated, abortHandler);

        writer->write_object_t(_FilterBankOrder, abortHandler);
        writer->write_object_t(_TimeResolution, abortHandler);
        writer->write_object_t(_IIRBandwidth, abortHandler);

        writer->write_object_t(_SuppressMirrorImage, abortHandler);

        // Version 17
        writer->write(&_FitMode, sizeof(_FitMode), abortHandler);

        // Version 18, v0.7.1.0-beta-2
        writer->write_object_t(_ShowArtworkOnBackground, abortHandler);

        writer->write_object_t(_GridRowCount, abortHandler);
        writer->write_object_t(_GridColumnCount, abortHandler);

        writer->write_object_t(GraphSettings::_CurentVersion, abortHandler);

        writer->write_object_t(_VerticalLayout, abortHandler);

        writer->write_object_t(_GraphSettings.size(), abortHandler);

        for (auto & gs : _GraphSettings)
        {
            pfc::string Description = pfc::utf8FromWide(gs._Description.c_str());
            writer->write_string(Description, abortHandler);

            writer->write_object_t(gs._Channels, abortHandler);
            writer->write_object_t(gs._FlipHorizontally, abortHandler);
            writer->write_object_t(gs._FlipVertically, abortHandler);

            writer->write_object (&gs._XAxisMode, sizeof(gs._XAxisMode), abortHandler);
            writer->write_object_t(gs._XAxisTop, abortHandler);
            writer->write_object_t(gs._XAxisBottom, abortHandler);

            writer->write_object (&gs._YAxisMode, sizeof(gs._YAxisMode), abortHandler);
            writer->write_object_t(gs._YAxisLeft, abortHandler);
            writer->write_object_t(gs._YAxisRight, abortHandler);

            writer->write_object_t(gs._AmplitudeLo, abortHandler);
            writer->write_object_t(gs._AmplitudeHi, abortHandler);
            writer->write_object_t(gs._AmplitudeStep, abortHandler);

            writer->write_object_t(gs._UseAbsolute, abortHandler);
            writer->write_object_t(gs._Gamma, abortHandler);

            writer->write_object_t(gs._HRatio, abortHandler);
            writer->write_object_t(gs._VRatio, abortHandler);

            // Version 2, v0.7.6.0
            if (GraphSettings::_CurentVersion > 1)
            {
                writer->write_object_t(gs._LPadding, abortHandler);
                writer->write_object_t(gs._RPadding, abortHandler);
                writer->write_object_t(gs._TPadding, abortHandler);
                writer->write_object_t(gs._BPadding, abortHandler);

                writer->write_object(&gs._HAlignment, sizeof(gs._HAlignment), abortHandler);
                writer->write_object(&gs._VAlignment, sizeof(gs._VAlignment), abortHandler);
            }
        }

        // Version 19, v0.7.2.0
        writer->write_object_t(_ConstantQ, abortHandler);
        writer->write_object_t(_CompensateBW, abortHandler);
        writer->write_object_t(_PreWarpQ, abortHandler);

        // Version 20
        {
            pfc::string Path;

            if (!isPreset)
                Path = pfc::utf8FromWide(_PresetsDirectoryPath.c_str());

            writer->write_string(Path, abortHandler);
        }

        // Version 21, v0.7.5.0-beta1
        writer->write_object_t(_ScrollingSpectogram, abortHandler);

        // Version 22, v0.7.5.0-beta2
        writer->write_object_t(_HorizontalPeakMeter, abortHandler);

        // Version 23, v0.7.5.0-beta3
        writer->write_object_t(_LEDSize, abortHandler);
        writer->write_object_t(_LEDGap, abortHandler);

        // Version 24, v0.7.5.2
        writer->write_object_t(_FitWindow, abortHandler);

        // Version 25, v0.7.5.3
        writer->write_object_t(_RMSWindow, abortHandler);

        // Version 26, v0.7.6.0
        writer->write_object_t(_GaugeGap, abortHandler);
        writer->write_object_t(_RMSPlus3, abortHandler);

        // Version 27, v0.8.0.0-beta1
        writer->write_object(&_ChannelPair, sizeof(_ChannelPair), abortHandler);
        writer->write_object_t(_HorizontalLevelMeter, abortHandler);
        writer->write_object_t(_HorizontalSpectogram, abortHandler);
        writer->write_object_t(_UseSpectrumBarMetrics, abortHandler);
    }
    catch (exception & ex)
    {
        Log::Write(Log::Level::Error, "%8d: %s failed to write CUI configuration: %s", (uint32_t) ::GetTickCount64(), core_api::get_my_file_name(), ex.what());
    }
}

/// <summary>
/// One time conversion of the old color settings.
/// </summary>
void State::ConvertColorSettings() noexcept
{
    {
        Style * style = _StyleManager.GetStyle(VisualElement::GraphBackground);

        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

        if ((_BackgroundMode_Deprecated > BackgroundMode::Artwork) && (_ArtworkGradientStops.size() > 0))
        {
            _BackgroundMode_Deprecated = BackgroundMode::Artwork;

            style->_ColorSource = ColorSource::DominantColor;
            style->_CurrentColor = _StyleManager._DominantColor;
        }
        else
        if (!_UseCustomBackColor_Deprecated)
        {
            style->_ColorSource = ColorSource::UserInterface;
            style->_CurrentColor = _StyleManager._UserInterfaceColors[_IsDUI ? 1U : 3U];
        }
        else
        {
            style->_CustomColor = _BackColor_Deprecated;

            style->_ColorSource = ColorSource::Solid;
            style->_CurrentColor = style->_CustomColor;
        }

        style->_Flags |= (_HorizontalGradient_Deprecated ? Style::HorizontalGradient : 0);
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::VerticalGridLine);

        style->_CustomColor = _XLineColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomXLineColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager._UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::XAxisText);

        style->_CustomColor = _XTextColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomXTextColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager._UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::HorizontalGridLine);

        style->_CustomColor = _YLineColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomYLineColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager._UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::YAxisText);

        style->_CustomColor = _YTextColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            if (!_UseCustomYTextColor_Deprecated)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_CurrentColor = _StyleManager._UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_CurrentColor = style->_CustomColor;
            }

        style->_CurrentGradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::BarArea);

        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

        style->_ColorScheme = _ColorScheme_Deprecated;

            style->_ColorSource = ColorSource::Gradient;
            style->_CurrentColor = D2D1::ColorF(0, 0.f);
            style->_CurrentGradientStops = SelectGradientStops_Deprecated(_ColorScheme_Deprecated);
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::BarDarkBackground);

        style->_CustomColor = _DarkBandColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

            style->_ColorSource = ColorSource::Solid;
            style->_CurrentColor = style->_CustomColor;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::BarLightBackground);

        style->_CustomColor = _LightBandColor_Deprecated;
        style->_CustomGradientStops = _CustomGradientStops_Deprecated;

            style->_ColorSource = ColorSource::Solid;
            style->_CurrentColor = style->_CustomColor;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::CurveLine);

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
        Style * style = _StyleManager.GetStyle(VisualElement::CurveArea);

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
void State::ConvertGraphSettings() noexcept
{
    for (auto & gs : _GraphSettings)
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
const GradientStops State::SelectGradientStops_Deprecated(ColorScheme colorScheme) const noexcept
{
    if (colorScheme == ColorScheme::Custom)
        return _CustomGradientStops_Deprecated;

    if (colorScheme == ColorScheme::Artwork)
        return _ArtworkGradientStops;

    return GetGradientStops(colorScheme);
}

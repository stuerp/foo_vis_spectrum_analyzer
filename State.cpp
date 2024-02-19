
/** $VER: State.cpp (2024.02.19) P. Stuer **/

#include "State.h"

#include "Gradients.h"
#include "Log.h"

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
    _UseToneGenerator = false;

    _DialogBounds = { };
    _PageIndex = 0;

    _RefreshRateLimit = 20;

    _UseHardwareRendering = true;
    _UseAntialiasing = true;

    _UseZeroTrigger = false;
    _WindowDuration = 50;

    // Transform
    _Transform = Transform::FFT;

    _WindowFunction = WindowFunctions::Hann;
    _WindowParameter = 1.;
    _WindowSkew = 0.;
    _Truncate = true;

    _ReactionAlignment = 0.25;

    _SelectedChannels = AllChannels;

    // FFT
    _FFTMode = FFTMode::FFT4096;
    _FFTCustom = 4096;
    _FFTDuration = 100.;

    _MappingMethod = Mapping::Standard;

    // CQT
    _CQTBandwidthOffset = 1.;
    _CQTAlignment = 1.;
    _CQTDownSample = 0.;

    // SWIFT
    _FilterBankOrder = 4;
    _TimeResolution = 600.;
    _SWIFTBandwidth = 1.;

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
    _BackColor = D2D1::ColorF(0.f, 0.f, 0.f, 1.f);
    _UseCustomBackColor = true;

    // X axis
    _XAxisMode = XAxisMode::Notes;
    _XAxisTop = true;
    _XAxisBottom = true;

    _XTextColor = D2D1::ColorF(D2D1::ColorF::White);
    _UseCustomXTextColor = true;

    _XLineColor = D2D1::ColorF(.25f, .25f, .25f, 1.f);
    _UseCustomXLineColor = true;

    // Y axis
    _YAxisMode = YAxisMode::Decibels;
    _YAxisLeft = true;
    _YAxisRight = true;

    _YTextColor = D2D1::ColorF(D2D1::ColorF::White);
    _UseCustomYTextColor = true;

    _YLineColor = D2D1::ColorF(.25f, .25f, .25f, 1.f);
    _UseCustomYLineColor = true;

    _AmplitudeLo = -90.;
    _AmplitudeHi =   0.;
    _AmplitudeStep = -6.;

    _UseAbsolute = true;
    _Gamma = 1.;

    // Common
    _ColorScheme = ColorScheme::Prism1;

    _GradientStops = GetGradientStops(_ColorScheme);
    _CustomGradientStops = GetGradientStops(ColorScheme::Custom);

    _ShowToolTips = true;
    _SuppressMirrorImage = true;

    _BackgroundMode = BackgroundMode::Artwork;
    _ArtworkOpacity = 1.f;
    _ArtworkFilePath.clear();
    _FitMode = FitMode::FitBig;

    _NumArtworkColors = 10;
    _LightnessThreshold = 250.f / 255.f;
    _TransparencyThreshold = 125.f / 255.f;

    _ColorOrder = ColorOrder::None;

    _VisualizationType = VisualizationType::Bars;

    // Bars
    _DrawBandBackground = true;
    _LightBandColor = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _DarkBandColor = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _LEDMode = false;
    _HorizontalGradient = false;

    _PeakMode = PeakMode::Classic;
    _HoldTime = 30.;
    _Acceleration = 0.5;

    // Curve
    _LineWidth = 2.f;
    _LineColor = D2D1::ColorF(D2D1::ColorF::White);
    _UseCustomLineColor = false;
    _PeakLineColor = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _UseCustomPeakLineColor = false;
    _AreaOpacity = 0.5f;

    _StyleManager.Reset();

    _GridRowCount = 1;
    _GridColumnCount = 2;

    _GraphSettings.clear();

    static const GraphSettings gs[] =
    {
//      { L"Stereo", audio_chunk::channel_config_2point1, false, true },

        { L"Left",  audio_chunk::channel_front_left,  .5f, 1.f, true,  false },
        { L"Right", audio_chunk::channel_front_right, .5f, 1.f, false, false },

    };

    for (const auto & Iter : gs)
        _GraphSettings.push_back(Iter);
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

    _UseZeroTrigger = other._UseZeroTrigger;
    _WindowDuration = other._WindowDuration;

    #pragma region Transform

        _Transform = other._Transform;

        _WindowFunction = other._WindowFunction;
        _WindowParameter = other._WindowParameter;
        _WindowSkew = other._WindowSkew;
        _Truncate = other._Truncate;

        _ReactionAlignment = other._ReactionAlignment;

        _SelectedChannels = other._SelectedChannels;

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

    #pragma region SWIFT

        _FilterBankOrder = other._FilterBankOrder;
        _TimeResolution = other._TimeResolution;
        _SWIFTBandwidth = other._SWIFTBandwidth;

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

        _BackColor = other._BackColor;
        _UseCustomBackColor = other._UseCustomBackColor;

        // X axis
        _XAxisMode = other._XAxisMode;
        _XAxisTop = other._XAxisTop;
        _XAxisBottom = other._XAxisBottom;

        _XTextColor = other._XTextColor;
        _UseCustomXTextColor = other._UseCustomXTextColor;

        _XLineColor = other._XLineColor;
        _UseCustomXLineColor = other._UseCustomXLineColor;

        // Y axis
        _YAxisMode = other._YAxisMode;
        _YAxisLeft = other._YAxisLeft;
        _YAxisRight = other._YAxisRight;

        _YTextColor = other._YTextColor;
        _UseCustomYTextColor = other._UseCustomYTextColor;

        _YLineColor = other._YLineColor;
        _UseCustomYLineColor = other._UseCustomYLineColor;

        _AmplitudeLo = other._AmplitudeLo;
        _AmplitudeHi = other._AmplitudeHi;
        _AmplitudeStep = other._AmplitudeStep;

        _UseAbsolute = other._UseAbsolute;

        _Gamma = other._Gamma;

    // Common
    _ColorScheme = other._ColorScheme;

    _SmoothingMethod = other._SmoothingMethod;
    _SmoothingFactor = other._SmoothingFactor;

    _GradientStops = other._GradientStops;
    _CustomGradientStops = other._CustomGradientStops;

    _ShowToolTips = other._ShowToolTips;
    _SuppressMirrorImage = other._SuppressMirrorImage;

    _NumArtworkColors = other._NumArtworkColors;
    _LightnessThreshold = other._LightnessThreshold;
    _TransparencyThreshold = other._TransparencyThreshold;

    _ColorOrder = other._ColorOrder;

    _BackgroundMode = other._BackgroundMode;
    _ArtworkOpacity = other._ArtworkOpacity;
    _ArtworkFilePath = other._ArtworkFilePath;
    _FitMode = other._FitMode;

    // Visualization
    _VisualizationType = other._VisualizationType;

    // Bars
    _DrawBandBackground = other._DrawBandBackground;
    _LightBandColor = other._LightBandColor;
    _DarkBandColor = other._DarkBandColor;
    _LEDMode = other._LEDMode;
    _HorizontalGradient = other._HorizontalGradient;

    _PeakMode = other._PeakMode;
    _HoldTime = other._HoldTime;
    _Acceleration = other._Acceleration;

    // Curve
    _LineWidth = other._LineWidth;
    _LineColor = other._LineColor;
    _UseCustomLineColor = other._UseCustomLineColor;
    _PeakLineColor = other._PeakLineColor;
    _UseCustomPeakLineColor = other._UseCustomPeakLineColor;
    _AreaOpacity = other._AreaOpacity;

    #pragma endregion

    _StyleManager = other._StyleManager;

    _GridRowCount = other._GridRowCount;
    _GridColumnCount = other._GridColumnCount;

    _GraphSettings = other._GraphSettings;

    #pragma region Not serialized

    _BinCount = other._BinCount;

    #pragma endregion

    return *this;
}

/// <summary>
/// Scales the specified value to a relative amplitude between 0.0 and 1.0.
/// </summary>
/// <remarks>FIXME: This should not live here but it's pretty convenient...</remarks>
double State::ScaleA(double value) const
{
    switch (_YAxisMode)
    {
        case YAxisMode::None:
            return 0.;

        default:

        case YAxisMode::Decibels:
            return Map(ToDecibel(value), _AmplitudeLo, _AmplitudeHi, 0.0, 1.0);

        case YAxisMode::Linear:
        {
            const double Exponent = 1.0 / _Gamma;

            return Map(::pow(value, Exponent), _UseAbsolute ? 0.0 : ::pow(ToMagnitude(_AmplitudeLo), Exponent), ::pow(ToMagnitude(_AmplitudeHi), Exponent), 0.0, 1.0);
        }
    }
}

/// <summary>
/// Reads this instance with the specified reader. (CUI version)
/// </summary>
void State::Read(stream_reader * reader, size_t size, abort_callback & abortHandler) noexcept
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

        reader->read(&_RefreshRateLimit, sizeof(_RefreshRateLimit), abortHandler); _RefreshRateLimit = Clamp<size_t>(_RefreshRateLimit, 20, 200);

        reader->read(&_UseHardwareRendering, sizeof(_UseHardwareRendering), abortHandler);
        reader->read(&_UseAntialiasing, sizeof(_UseAntialiasing), abortHandler);

        reader->read(&_UseZeroTrigger, sizeof(_UseZeroTrigger), abortHandler);

        reader->read(&_WindowDuration, sizeof(_WindowDuration), abortHandler); _WindowDuration = Clamp<size_t>(_WindowDuration, 50, 800);

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

        reader->read(&_BackColor, sizeof(_BackColor), abortHandler);

        reader->read(&_XAxisMode, sizeof(_XAxisMode), abortHandler);

        reader->read(&_YAxisMode, sizeof(_YAxisMode), abortHandler);

        reader->read(&_AmplitudeLo, sizeof(_AmplitudeLo), abortHandler);
        reader->read(&_AmplitudeHi, sizeof(_AmplitudeHi), abortHandler);
        reader->read(&_UseAbsolute, sizeof(_UseAbsolute), abortHandler);
        reader->read(&_Gamma, sizeof(_Gamma), abortHandler);

        reader->read(&_ColorScheme, sizeof(_ColorScheme), abortHandler);

        if ((Version <= 9) && (_ColorScheme != ColorScheme::Solid) && (_ColorScheme != ColorScheme::Custom))
            _ColorScheme = (ColorScheme) ((int) _ColorScheme + 1); // ColorScheme::Artwork was added after ColorScheme::Custom

        reader->read(&_DrawBandBackground, sizeof(_DrawBandBackground), abortHandler);

        reader->read(&_PeakMode, sizeof(_PeakMode), abortHandler);
        reader->read(&_HoldTime, sizeof(_HoldTime), abortHandler);
        reader->read(&_Acceleration, sizeof(_Acceleration), abortHandler);

    #pragma endregion

        _CustomGradientStops.clear();

        size_t Count; reader->read(&Count, sizeof(Count), abortHandler);

        for (size_t i = 0; i < Count; ++i)
        {
            D2D1_GRADIENT_STOP gs = { };

            reader->read(&gs.position, sizeof(gs.position), abortHandler);
            reader->read(&gs.color, sizeof(gs.color), abortHandler);

            _CustomGradientStops.push_back(gs);
        }

        reader->read(&_XTextColor, sizeof(_XTextColor), abortHandler);
        reader->read(&_XLineColor, sizeof(_XLineColor), abortHandler);
        reader->read(&_YTextColor, sizeof(_YTextColor), abortHandler);
        reader->read(&_YLineColor, sizeof(_YLineColor), abortHandler);
        reader->read(&_DarkBandColor, sizeof(_DarkBandColor), abortHandler);

        reader->read(&_AmplitudeStep, sizeof(_AmplitudeStep), abortHandler);

        reader->read(&_SelectedChannels, sizeof(_SelectedChannels), abortHandler);
        reader->read(&_ShowToolTips, sizeof(_ShowToolTips), abortHandler);

        reader->read(&_WindowFunction, sizeof(_WindowFunction), abortHandler);
        reader->read(&_WindowParameter, sizeof(_WindowParameter), abortHandler);
        reader->read(&_WindowSkew, sizeof(_WindowSkew), abortHandler);

        if (Version >= 8)
        {
            reader->read(&_UseCustomBackColor, sizeof(_UseCustomBackColor), abortHandler);
            reader->read(&_UseCustomXTextColor, sizeof(_UseCustomXTextColor), abortHandler);
            reader->read(&_UseCustomXLineColor, sizeof(_UseCustomXLineColor), abortHandler);
            reader->read(&_UseCustomYTextColor, sizeof(_UseCustomYTextColor), abortHandler);
            reader->read(&_UseCustomYLineColor, sizeof(_UseCustomYLineColor), abortHandler);

            reader->read(&_LEDMode, sizeof(_LEDMode), abortHandler);

            reader->read(&_HorizontalGradient, sizeof(_HorizontalGradient), abortHandler);

            reader->read(&_LightBandColor, sizeof(_LightBandColor), abortHandler);
        }

        if (Version >= 9)
        {
            reader->read(&_PageIndex, sizeof(_PageIndex), abortHandler);
            reader->read(&_VisualizationType, sizeof(_VisualizationType), abortHandler);
            reader->read(&_LineWidth, sizeof(_LineWidth), abortHandler);
            reader->read(&_AreaOpacity, sizeof(_AreaOpacity), abortHandler);
        }

        if (Version >= 10)
        {
            reader->read(&_BackgroundMode, sizeof(_BackgroundMode), abortHandler); _BackgroundMode = Clamp(_BackgroundMode, BackgroundMode::None, BackgroundMode::Artwork);
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

            reader->read(&_LineColor, sizeof(_LineColor), abortHandler);
            reader->read(&_UseCustomLineColor, sizeof(_UseCustomLineColor), abortHandler);
            reader->read(&_PeakLineColor, sizeof(_PeakLineColor), abortHandler);
            reader->read(&_UseCustomPeakLineColor, sizeof(_UseCustomPeakLineColor), abortHandler);
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
            _ArtworkFilePath = reader->read_string(abortHandler);

        if (Version >= 14)
            _StyleManager.Read(reader, size, abortHandler);

        if (Version >= 16)
        {
            reader->read_object_t(_ReactionAlignment, abortHandler);

            reader->read_object_t(_XAxisTop, abortHandler);
            reader->read_object_t(_XAxisBottom, abortHandler);
            reader->read_object_t(_YAxisLeft, abortHandler);
            reader->read_object_t(_YAxisRight, abortHandler);

            reader->read_object_t(_FilterBankOrder, abortHandler);
            reader->read_object_t(_TimeResolution, abortHandler);
            reader->read_object_t(_SWIFTBandwidth, abortHandler);

            reader->read_object_t(_SuppressMirrorImage, abortHandler);
        }

        if (Version >= 17)
        {
            reader->read(&_FitMode, sizeof(_FitMode), abortHandler);
        }
    }
    catch (exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing DUI configuration data: %s", core_api::get_my_file_name(), ex.what());

        Reset();
    }
}

/// <summary>
/// Writes this instance to the specified writer. (CUI version)
/// </summary>
void State::Write(stream_writer * writer, abort_callback & abortHandler) const noexcept
{
    try
    {
        writer->write(&_CurrentVersion, sizeof(_CurrentVersion), abortHandler);

        #pragma region User Interface

        writer->write(&_DialogBounds, sizeof(_DialogBounds), abortHandler);

        writer->write(&_RefreshRateLimit, sizeof(_RefreshRateLimit), abortHandler);

        writer->write(&_UseHardwareRendering, sizeof(_UseHardwareRendering), abortHandler);
        writer->write(&_UseAntialiasing, sizeof(_UseAntialiasing), abortHandler);

        writer->write(&_UseZeroTrigger, sizeof(_UseZeroTrigger), abortHandler);
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

        writer->write(&_BackColor, sizeof(_BackColor), abortHandler);

        writer->write(&_XAxisMode, sizeof(_XAxisMode), abortHandler);

        writer->write(&_YAxisMode, sizeof(_YAxisMode), abortHandler);

        writer->write(&_AmplitudeLo, sizeof(_AmplitudeLo), abortHandler);
        writer->write(&_AmplitudeHi, sizeof(_AmplitudeHi), abortHandler);
        writer->write(&_UseAbsolute, sizeof(_UseAbsolute), abortHandler);
        writer->write(&_Gamma, sizeof(_Gamma), abortHandler);

        writer->write(&_ColorScheme, sizeof(_ColorScheme), abortHandler);

        writer->write(&_DrawBandBackground, sizeof(_DrawBandBackground), abortHandler);

        writer->write(&_PeakMode, sizeof(_PeakMode), abortHandler);
        writer->write(&_HoldTime, sizeof(_HoldTime), abortHandler);
        writer->write(&_Acceleration, sizeof(_Acceleration), abortHandler);

        #pragma endregion

        size_t Size = _CustomGradientStops.size();

        writer->write(&Size, sizeof(Size), abortHandler);

        for (const auto & Iter : _CustomGradientStops)
        {
            writer->write(&Iter.position, sizeof(Iter.position), abortHandler);
            writer->write(&Iter.color, sizeof(Iter.color), abortHandler);
        }

        writer->write(&_XTextColor, sizeof(_XTextColor), abortHandler);
        writer->write(&_XLineColor, sizeof(_XLineColor), abortHandler);
        writer->write(&_YTextColor, sizeof(_YTextColor), abortHandler);
        writer->write(&_YLineColor, sizeof(_YLineColor), abortHandler);
        writer->write(&_DarkBandColor, sizeof(_DarkBandColor), abortHandler);

        writer->write(&_AmplitudeStep, sizeof(_AmplitudeStep), abortHandler);

        writer->write(&_SelectedChannels, sizeof(_SelectedChannels), abortHandler);
        writer->write(&_ShowToolTips, sizeof(_ShowToolTips), abortHandler);

        writer->write(&_WindowFunction, sizeof(_WindowFunction), abortHandler);
        writer->write(&_WindowParameter, sizeof(_WindowParameter), abortHandler);
        writer->write(&_WindowSkew, sizeof(_WindowSkew), abortHandler);

        // Version 8
        writer->write(&_UseCustomBackColor,  sizeof(_UseCustomBackColor), abortHandler);
        writer->write(&_UseCustomXTextColor, sizeof(_UseCustomXTextColor), abortHandler);
        writer->write(&_UseCustomXLineColor, sizeof(_UseCustomXLineColor), abortHandler);
        writer->write(&_UseCustomYTextColor, sizeof(_UseCustomYTextColor), abortHandler);
        writer->write(&_UseCustomYLineColor, sizeof(_UseCustomYLineColor), abortHandler);

        writer->write(&_LEDMode, sizeof(_LEDMode), abortHandler);

        writer->write(&_HorizontalGradient, sizeof(_HorizontalGradient), abortHandler);

        writer->write(&_LightBandColor, sizeof(_LightBandColor), abortHandler);

        // Version 9
        writer->write(&_PageIndex, sizeof(_PageIndex), abortHandler);
        writer->write(&_VisualizationType, sizeof(_VisualizationType), abortHandler);
        writer->write(&_LineWidth, sizeof(_LineWidth), abortHandler);
        writer->write(&_AreaOpacity, sizeof(_AreaOpacity), abortHandler);

        // Version 10
        writer->write(&_BackgroundMode, sizeof(_BackgroundMode), abortHandler);
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

        writer->write(&_LineColor, sizeof(_LineColor), abortHandler);
        writer->write(&_UseCustomLineColor, sizeof(_UseCustomLineColor), abortHandler);
        writer->write(&_PeakLineColor, sizeof(_PeakLineColor), abortHandler);
        writer->write(&_UseCustomPeakLineColor, sizeof(_UseCustomPeakLineColor), abortHandler);

        // Version 12
        writer->write(&_BandwidthOffset, sizeof(_BandwidthOffset), abortHandler);
        writer->write(&_BandwidthCap, sizeof(_BandwidthCap), abortHandler);
        writer->write(&_BandwidthAmount, sizeof(_BandwidthAmount), abortHandler);
        writer->write(&_UseGranularBandwidth, sizeof(_UseGranularBandwidth), abortHandler);

        writer->write(&_KernelShape, sizeof(_KernelShape), abortHandler);
        writer->write(&_KernelShapeParameter, sizeof(_KernelShapeParameter), abortHandler);
        writer->write(&_KernelAsymmetry, sizeof(_KernelAsymmetry), abortHandler);

        // Version 13
        writer->write_string(_ArtworkFilePath, abortHandler);

        // Version 15
        _StyleManager.Write(writer, abortHandler);

        // Version 16
        writer->write_object_t(_ReactionAlignment, abortHandler);

        writer->write_object_t(_XAxisTop, abortHandler);
        writer->write_object_t(_XAxisBottom, abortHandler);
        writer->write_object_t(_YAxisLeft, abortHandler);
        writer->write_object_t(_YAxisRight, abortHandler);

        writer->write_object_t(_FilterBankOrder, abortHandler);
        writer->write_object_t(_TimeResolution, abortHandler);
        writer->write_object_t(_SWIFTBandwidth, abortHandler);

        writer->write_object_t(_SuppressMirrorImage, abortHandler);

        // Version 17
        writer->write(&_FitMode, sizeof(_FitMode), abortHandler);
    }
    catch (exception & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing CUI configuration data: %s", core_api::get_my_file_name(), ex.what());
    }
}

/// <summary>
/// One time conversion of old color settings.
/// </summary>
void State::ConvertColorSettings() noexcept
{
    {
        Style * style = _StyleManager.GetStyle(VisualElement::GraphBackground);

        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

        if ((_BackgroundMode > BackgroundMode::Artwork) && (_ArtworkGradientStops.size() > 0))
        {
            _BackgroundMode = BackgroundMode::Artwork;

            style->_ColorSource = ColorSource::DominantColor;
            style->_Color = _DominantColor;
        }
        else
        if (!_UseCustomBackColor)
        {
            style->_ColorSource = ColorSource::UserInterface;
            style->_Color = _UserInterfaceColors[_IsDUI ? 1U : 3U];
        }
        else
        {
            style->_CustomColor = _BackColor;

            style->_ColorSource = ColorSource::Solid;
            style->_Color = style->_CustomColor;
        }

        style->_Flags |= (_HorizontalGradient ? Style::HorizontalGradient : 0);
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::XAxisLine);

        style->_CustomColor = _XLineColor;
        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

            if (!_UseCustomXLineColor)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_Color = _UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_Color = style->_CustomColor;
            }

        style->_GradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::XAxisText);

        style->_CustomColor = _XTextColor;
        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

            if (!_UseCustomXTextColor)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_Color = _UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_Color = style->_CustomColor;
            }

        style->_GradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::YAxisLine);

        style->_CustomColor = _YLineColor;
        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

            if (!_UseCustomYLineColor)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_Color = _UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_Color = style->_CustomColor;
            }

        style->_GradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::YAxisText);

        style->_CustomColor = _YTextColor;
        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

            if (!_UseCustomYTextColor)
            {
                style->_ColorSource = ColorSource::UserInterface;
                style->_Color = _UserInterfaceColors[0];
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_Color = style->_CustomColor;
            }

        style->_GradientStops = style->_CustomGradientStops;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::BarSpectrum);

        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

            style->_ColorSource = ColorSource::Gradient;
            style->_Color = D2D1::ColorF(0, 0.f);
            style->_GradientStops = SelectGradientStops(_ColorScheme);
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::BarDarkBackground);

        style->_CustomColor = _DarkBandColor;
        style->_CustomGradientStops = _CustomGradientStops;

            style->_ColorSource = ColorSource::Solid;
            style->_Color = style->_CustomColor;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::BarLightBackground);

        style->_CustomColor = _LightBandColor;
        style->_CustomGradientStops = _CustomGradientStops;

            style->_ColorSource = ColorSource::Solid;
            style->_Color = style->_CustomColor;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::CurveLine);

        style->_CustomColor = _LineColor;
        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

            if (!_UseCustomLineColor)
            {
                style->_ColorSource = ColorSource::Gradient;
                style->_Color = D2D1::ColorF(0, 0.f);
                style->_GradientStops = SelectGradientStops(_ColorScheme);
            }
            else
            {
                style->_ColorSource = ColorSource::Solid;
                style->_Color = style->_CustomColor;
            }

        style->_Thickness = _LineWidth;
    }

    {
        Style * style = _StyleManager.GetStyle(VisualElement::CurveArea);

        style->_CustomGradientStops = _CustomGradientStops;

        style->_ColorScheme = _ColorScheme;

            style->_ColorSource = ColorSource::Gradient;
            style->_Color = D2D1::ColorF(0, 0.f);
            style->_GradientStops = SelectGradientStops(_ColorScheme);
            style->_Opacity = _AreaOpacity;
    }
}

/// <summary>
/// Helper method to initialize the gradient stops vector during conversion.
/// </summary>
const GradientStops State::SelectGradientStops(ColorScheme colorScheme) const noexcept
{
    if (colorScheme == ColorScheme::Custom)
        return _CustomGradientStops;

    if (colorScheme == ColorScheme::Artwork)
        return _ArtworkGradientStops;

    return GetGradientStops(colorScheme);
}

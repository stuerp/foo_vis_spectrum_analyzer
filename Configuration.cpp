
/** $VER: Configuration.cpp (2023.12.14) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

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
    _DialogBounds = { };

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

    _SelectedChannels = AllChannels;

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
    _UseCustomBackColor = true;

    // X axis
    _XAxisMode = XAxisMode::Notes;

    _XTextColor = D2D1::ColorF(D2D1::ColorF::White);
    _UseCustomXTextColor = true;

    _XLineColor = D2D1::ColorF(.25f, .25f, .25f, 1.f);
    _UseCustomXLineColor = true;

    // Y axis
    _YAxisMode = YAxisMode::Decibels;

    _YTextColor = D2D1::ColorF(D2D1::ColorF::White);
    _UseCustomYTextColor = true;

    _YLineColor = D2D1::ColorF(.25f, .25f, .25f, 1.f);
    _UseCustomYLineColor = true;

    _AmplitudeLo = -90.;
    _AmplitudeHi =   0.;
    _AmplitudeStep = -6.;

    _UseAbsolute = true;
    _Gamma = 1.;

    // Band
    _DrawBandBackground = true;
    _LiteBandColor = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _DarkBandColor = D2D1::ColorF(.2f, .2f, .2f, .7f);
    _LEDMode = false;
    _ShowToolTips = true;
    _HorizontalGradient = false;

    _ColorScheme = ColorScheme::Prism1;

    _GradientStops = GetGradientStops(_ColorScheme);
    _CustomGradientStops = GetGradientStops(ColorScheme::Custom);

    _PeakMode = PeakMode::Classic;
    _HoldTime = 30.;
    _Acceleration = 0.5;

    // Logging
    _LogLevel = LogLevel::None;
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

    #pragma region Transform
        _Transform = other._Transform;

        _WindowFunction = other._WindowFunction;
        _WindowParameter = other._WindowParameter;
        _WindowSkew = other._WindowSkew;
        _Truncate = other._Truncate;

        _SelectedChannels = other._SelectedChannels;
    #pragma endregion

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
        _UseCustomBackColor = other._UseCustomBackColor;

        // X axis
        _XAxisMode = other._XAxisMode;

        _XTextColor = other._XTextColor;
        _UseCustomXTextColor = other._UseCustomXTextColor;

        _XLineColor = other._XLineColor;
        _UseCustomXLineColor = other._UseCustomXLineColor;

        // Y axis
        _YAxisMode = other._YAxisMode;

        _YTextColor = other._YTextColor;
        _UseCustomYTextColor = other._UseCustomYTextColor;

        _YLineColor = other._YLineColor;
        _UseCustomYLineColor = other._UseCustomYLineColor;

        _AmplitudeLo = other._AmplitudeLo;
        _AmplitudeHi = other._AmplitudeHi;
        _AmplitudeStep = other._AmplitudeStep;

        _UseAbsolute = other._UseAbsolute;

        _Gamma = other._Gamma;

        // Bands
        _DrawBandBackground = other._DrawBandBackground;
        _LiteBandColor = other._LiteBandColor;
        _DarkBandColor = other._DarkBandColor;
        _LEDMode = other._LEDMode;
        _ShowToolTips = other._ShowToolTips;
        _HorizontalGradient = other._HorizontalGradient;

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
/// Reads this instance from the specified parser. (DUI version)
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

    // Reduce the size to make sure it fits on screens scaled to 150%.
    if ((_DialogBounds.right - _DialogBounds.left) > 1910)
        _DialogBounds.right = _DialogBounds.left + 1910;

    if ((_DialogBounds.bottom - _DialogBounds.top) > 995)
        _DialogBounds.bottom = _DialogBounds.top + 995;

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

    parser >> _DarkBandColor.r;
    parser >> _DarkBandColor.g;
    parser >> _DarkBandColor.b;
    parser >> _DarkBandColor.a;

    // Version 6
    if (Version >= 6)
        parser >> _AmplitudeStep;

    // Version 7
    if (Version >= 7)
    {
        parser >> _SelectedChannels;
        parser >> _ShowToolTips;

        parser >> Integer; _WindowFunction = (WindowFunctions) Integer;
        parser >> _WindowParameter;
        parser >> _WindowSkew;
    }

    // Version 8
    if (Version >= 8)
    {
        parser >> _UseCustomBackColor;
        parser >> _UseCustomXTextColor;
        parser >> _UseCustomXLineColor;
        parser >> _UseCustomYTextColor;
        parser >> _UseCustomYLineColor;

        parser >> _LEDMode;

        parser >> _HorizontalGradient;

        parser >> _LiteBandColor.r;
        parser >> _LiteBandColor.g;
        parser >> _LiteBandColor.b;
        parser >> _LiteBandColor.a;
    }

    if (_ColorScheme != ColorScheme::Custom)
        _GradientStops = GetGradientStops(_ColorScheme);
    else
        _GradientStops = _CustomGradientStops;
}

/// <summary>
/// Writes this instance to the specified builder. (DUI version)
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

    builder << _DarkBandColor.r;
    builder << _DarkBandColor.g;
    builder << _DarkBandColor.b;
    builder << _DarkBandColor.a;

    // Version 6
    builder << _AmplitudeStep;

    // Version 7
    builder << _SelectedChannels;
    builder << _ShowToolTips;

    builder << (int) _WindowFunction;
    builder << _WindowParameter;
    builder << _WindowSkew;

    // Version 8
    builder << _UseCustomBackColor;
    builder << _UseCustomXTextColor;
    builder << _UseCustomXLineColor;
    builder << _UseCustomYTextColor;
    builder << _UseCustomYLineColor;

    builder << _LEDMode;

    builder << _HorizontalGradient;

    builder << _LiteBandColor.r;
    builder << _LiteBandColor.g;
    builder << _LiteBandColor.b;
    builder << _LiteBandColor.a;
}

/// <summary>
/// Reads this instance with the specified reader. (CUI version)
/// </summary>
void Configuration::Read(stream_reader * reader, size_t size, abort_callback & abortHandler)
{
    Reset();

    if (size < sizeof(size_t)) return;

    try
    {
        size_t Version;

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
        reader->read(&_FFTSize, sizeof(_FFTSize), abortHandler);
        reader->read(&_FFTCustom, sizeof(_FFTCustom), abortHandler);
        reader->read(&_FFTDuration, sizeof(_FFTDuration), abortHandler);

        reader->read(&_MappingMethod, sizeof(_MappingMethod), abortHandler);
        reader->read(&_SmoothingMethod, sizeof(_SmoothingMethod), abortHandler);
        reader->read(&_SmoothingFactor, sizeof(_SmoothingFactor), abortHandler);
        reader->read(&_KernelSize, sizeof(_KernelSize), abortHandler);
        reader->read(&_SummationMethod, sizeof(_SummationMethod), abortHandler);
        reader->read(&_SmoothLowerFrequencies, sizeof(_SmoothLowerFrequencies), abortHandler);
        reader->read(&_SmoothGainTransition, sizeof(_SmoothGainTransition), abortHandler);
#pragma endregion

#pragma region Frequencies
        reader->read(&_FrequencyDistribution, sizeof(_FrequencyDistribution), abortHandler);

        reader->read(&_NumBands, sizeof(_NumBands), abortHandler);

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

            reader->read(&_LiteBandColor, sizeof(_LiteBandColor), abortHandler);
        }

        if (_ColorScheme != ColorScheme::Custom)
            _GradientStops = GetGradientStops(_ColorScheme);
        else
            _GradientStops = _CustomGradientStops;
    }
    catch (...)
    {
        Reset();
    }
}

/// <summary>
/// Writes this instance to the specified writer. (CUI version)
/// </summary>
void Configuration::Write(stream_writer * writer, abort_callback & abortHandler) const
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
        writer->write(&_FFTSize, sizeof(_FFTSize), abortHandler);
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

        writer->write(&_NumBands, sizeof(_NumBands), abortHandler);
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

    writer->write(&_LiteBandColor, sizeof(_LiteBandColor), abortHandler);
}

/// <summary>
/// Updates the position of the current gradient colors.
/// </summary>
void Configuration::UpdateGradient()
{
    if (_GradientStops.size() == 0)
        return;

    if (_GradientStops.size() > 1)
    {
        FLOAT Position = 0.f;

        for (auto & Iter : _GradientStops)
        {
            Iter.position = Position / (FLOAT) (_GradientStops.size() - 1);
            Position++;
        }
    }
    else
        _GradientStops[0].position = 1.f;

    _ColorScheme = ColorScheme::Custom;
    _CustomGradientStops = _GradientStops;
}


/** $VER: Configuration.cpp (2023.11.18) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"

#include "Configuration.h"
#include "Support.h"
#include "Resources.h"
#include "Math.h"

#include "JSON.h"

using namespace JSON;

#include <tchar.h>
#include <pathcch.h>

#pragma comment(lib, "pathcch")

#include <pfc/string_conv.h>
#include <pfc/string-conv-lite.h>

using namespace pfc;
using namespace stringcvt;

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Configuration::Configuration()
{
    Reset();
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

    _FFTSize = other._FFTSize;

    _FrequencyDistribution = other._FrequencyDistribution;

    _NumBands = other._NumBands;
    _MinFrequency = other._MinFrequency;
    _MaxFrequency = other._MaxFrequency;

    // Note range
    _MinNote = other._MinNote;
    _MaxNote = other._MaxNote;
    _BandsPerOctave = other._BandsPerOctave;
    _Pitch = other._Pitch;
    _Transpose = other._Transpose;

    // Frequencies
    _ScalingFunction = other._ScalingFunction;

    _SkewFactor = other._SkewFactor;
    _Bandwidth = other._Bandwidth;

    _SmoothingMethod = other._SmoothingMethod;
    _SmoothingFactor = other._SmoothingFactor;

    _KernelSize = other._KernelSize;
    _SummationMethod = other._SummationMethod;
    _SmoothLowerFrequencies = other._SmoothLowerFrequencies;
    _SmoothGainTransition = other._SmoothGainTransition;

    // Rendering parameters
    _BackgroundColor = other._BackgroundColor;

    // X axis
    _XAxisMode = other._XAxisMode;

    // Y axis
    _YAxisMode = other._YAxisMode;

    _MinDecibel = other._MinDecibel;
    _MaxDecibel = other._MaxDecibel;

    _UseAbsolute = other._UseAbsolute;
    _Gamma = other._Gamma;

    // Band
    _ColorScheme = other._ColorScheme;
    _PeakMode = other._PeakMode;
    _DrawBandBackground = other._DrawBandBackground;

    // Logging
    _LogLevel = other._LogLevel;

    return *this;
}

/// <summary>
/// Resets this instance.
/// </summary>
void Configuration::Reset() noexcept
{
    _DialogBounds = {  };

    _RefreshRateLimit = 20;

    _UseHardwareRendering = true;
    _UseAntialiasing = true;

    _UseZeroTrigger = false;
    _WindowDuration = 100;

    _FFTSize = 4096;
    _FrequencyDistribution = FrequencyDistribution::Octaves;

    // Frequency range
    _NumBands = 320;
    _MinFrequency = 20;
    _MaxFrequency = 20000;

    // Note range
    _MinNote = 0;
    _MaxNote = 143;
    _BandsPerOctave = 12;
    _Pitch = 440.0;
    _Transpose = 0;

    // Frequencies
    _ScalingFunction = ScalingFunction::Logarithmic;

    _SkewFactor = 0.0;   // Hz linear factor, 0.0 .. 1.0
    _Bandwidth = 0.5;        // Bandwidth, 0.0 .. 64.0

    _SmoothingMethod = SmoothingMethod::Peak;
    _SmoothingFactor = 0.5;                         // Smoothing constant, 0.0 .. 1.0

    _KernelSize = 32;                               // Lanczos interpolation kernel size, 1 .. 64
    _SummationMethod = SummationMethod::Maximum;    // Band power summation method
    _SmoothLowerFrequencies = true;
    _SmoothGainTransition = true;

    // Rendering parameters
    _BackgroundColor = D2D1::ColorF(0.f, 0.f, 0.f, 1.f);

    // X axis
    _XAxisMode = XAxisMode::Notes;

    // Y axis
    _YAxisMode = YAxisMode::Decibels;

    _MinDecibel = -90.;
    _MaxDecibel =   0.;

    _UseAbsolute = true;                               // Use absolute value
    _Gamma = 1.;

    // Band
    _ColorScheme = ColorScheme::Prism1;
    _PeakMode = PeakMode::Classic;
    _DrawBandBackground = true;

    // Logging
    _LogLevel = LogLevel::None;
/*
    type: 'fft',
    bandwidthOffset: 1,

    windowFunction: 'hann',
    windowParameter: 1,
    windowSkew: 0,

    timeAlignment: 1,
    downsample: 0,

    useComplex: true,

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
}

/// <summary>
/// Reads this instance from the specified parser.
/// </summary>
void Configuration::Read(ui_element_config_parser & parser)
{
    Reset();

    try
    {
        size_t Version;

        parser >> Version;

        switch (Version)
        {
            case 1:
            {
                int Integer;

                parser >> _DialogBounds.left;
                parser >> _DialogBounds.top;
                parser >> _DialogBounds.right;
                parser >> _DialogBounds.bottom;

                parser >> _RefreshRateLimit; _RefreshRateLimit = Clamp<size_t>(_RefreshRateLimit, 20, 200);

                parser >> _UseHardwareRendering;
                parser >> _UseAntialiasing;

                parser >> _UseZeroTrigger;

                parser >> _WindowDuration; _WindowDuration = Clamp<size_t>(_WindowDuration, 50, 800);

            #pragma region FFT
                parser >> _Configuration._FFTSize;

                parser >> Integer; _Configuration._SmoothingMethod = (SmoothingMethod) Integer;
                parser >> _Configuration._SmoothingFactor;
                parser >> _Configuration._KernelSize;
                parser >> Integer; _Configuration._SummationMethod = (SummationMethod) Integer;
                parser >> _Configuration._SmoothLowerFrequencies;
                parser >> _Configuration._SmoothGainTransition;
                parser >> _Configuration._Gamma;
            #pragma endregion

            #pragma region Frequencies
                parser >> Integer; _Configuration._FrequencyDistribution = (FrequencyDistribution) Integer;

                parser >> _Configuration._NumBands;
                parser >> _Configuration._MinFrequency;
                parser >> _Configuration._MaxFrequency;

                parser >> _Configuration._MinNote;
                parser >> _Configuration._MaxNote;
                parser >> _Configuration._BandsPerOctave;
                parser >> _Configuration._Pitch;
                parser >> _Configuration._Transpose;

                parser >> Integer; _Configuration._ScalingFunction = (ScalingFunction) Integer;
                parser >> _Configuration._SkewFactor;
                parser >> _Configuration._Bandwidth;
            #pragma endregion

            #pragma region Rendering
                UINT32 Rgb;
                FLOAT Alpha;

                parser >> Rgb;
                parser >> Alpha;
                _Configuration._BackgroundColor = D2D1::ColorF(Rgb, Alpha);

                parser >> Integer; _Configuration._XAxisMode = (XAxisMode) Integer;

                parser >> Integer; _Configuration._YAxisMode = (YAxisMode) Integer;

                parser >> _Configuration._MinDecibel;
                parser >> _Configuration._MaxDecibel;
                parser >> _Configuration._UseAbsolute;

                parser >> Integer; _Configuration._ColorScheme = (ColorScheme) Integer;

                parser >> _Configuration._DrawBandBackground;

                parser >> Integer; _Configuration._PeakMode = (PeakMode) Integer;
                parser >> _Configuration._HoldTime;
                parser >> _Configuration._Acceleration;
            #pragma endregion
                break;
            }

            default:
                Log(LogLevel::Error, "%s: Unknown configuration format. Version: %d", core_api::get_my_file_name(), Version);
        }
    }
    catch (exception_io & ex)
    {
        Log(LogLevel::Error, "%s: Exception while reading configuration data: %s", core_api::get_my_file_name(), ex.what());
    }
}

/// <summary>
/// Writes this instance to the specified builder.
/// </summary>
void Configuration::Write(ui_element_config_builder & builder) const
{
    builder << _Version;

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

    #pragma region FFT
        builder << _Configuration._FFTSize;
        builder << (int) _Configuration._SmoothingMethod;
        builder << _Configuration._SmoothingFactor;
        builder << _Configuration._KernelSize;
        builder << (int) _Configuration._SummationMethod;
        builder << _Configuration._SmoothLowerFrequencies;
        builder << _Configuration._SmoothGainTransition;
        builder << _Configuration._Gamma;
    #pragma endregion

    #pragma region Frequencies
        builder << (int) _Configuration._FrequencyDistribution;

        builder << _Configuration._NumBands;
        builder << _Configuration._MinFrequency;
        builder << _Configuration._MaxFrequency;

        builder << _Configuration._MinNote;
        builder << _Configuration._MaxNote;
        builder << _Configuration._BandsPerOctave;
        builder << _Configuration._Pitch;
        builder << _Configuration._Transpose;

        builder << (int) _Configuration._ScalingFunction;
        builder << _Configuration._SkewFactor;
        builder << _Configuration._Bandwidth;
    #pragma endregion

    #pragma region Rendering
        builder << RGB((BYTE) (_Configuration._BackgroundColor.r * 255.f), (BYTE) (_Configuration._BackgroundColor.g * 255.f), (BYTE) (_Configuration._BackgroundColor.b * 255.f));
        builder << _Configuration._BackgroundColor.a;

        builder << (int) _Configuration._XAxisMode;

        builder << (int) _Configuration._YAxisMode;

        builder << _Configuration._MinDecibel;
        builder << _Configuration._MaxDecibel;
        builder << _Configuration._UseAbsolute;

        builder << (int) _Configuration._ColorScheme;

        builder << _Configuration._DrawBandBackground;

        builder << (int) _Configuration._PeakMode;
        builder << _Configuration._HoldTime;
        builder << _Configuration._Acceleration;
    #pragma endregion
}

/// <summary>
/// Reads the configuration from a JSON file.
/// </summary>
void Configuration::Read()
{
/*
    HMODULE hModule = ::GetModuleHandleW(TEXT(STR_COMPONENT_FILENAME));

    WCHAR PathName[MAX_PATH];

    ::GetModuleFileNameW(hModule, PathName, _countof(PathName));

    HRESULT hResult = ::PathCchRenameExtension(PathName, _countof(PathName), L"json");

    if (SUCCEEDED(hResult))
    {
        FILE * fp = ::_wfopen(PathName, L"r");

        if (fp != nullptr)
        {
            struct _stat64i32 Stat = { };

            ::_wstat(PathName, &Stat);

            char * UTF8 = new char[(size_t) Stat.st_size];

            if (UTF8 != nullptr)
            {
                size_t BytesRead = ::fread(UTF8, 1, (size_t) Stat.st_size, fp);

                wchar_t * Text = new wchar_t[BytesRead + 64];

                if (Text != nullptr)
                {
                    ::convert_utf8_to_wide(Text, BytesRead + 64, UTF8, BytesRead);

                    Reader Reader(Text);

                    Value Value;

                    try
                    {
                        bool Success = Reader.Read(Value);

                        while (Success)
                        {
                            if (Value.Contains(L"FFTSize"))
                            {
                                size_t v = (size_t) (int) Value[L"FFTSize"];

                                if (v >= (size_t) FFTSize::FFT64 && v <= (size_t) FFTSize::TimeWindow)
                                    _Configuration._FFTSize = v;
                            }

                            if (Value.Contains(L"FrequencyDistribution"))
                            {
                                FrequencyDistribution v = (FrequencyDistribution) (int) Value[L"FrequencyDistribution"];

                                if (v >= FrequencyDistribution::Frequencies && v <= FrequencyDistribution::AveePlayer)
                                    _Configuration._FrequencyDistribution = v;
                            }

                            if (Value.Contains(L"ScalingFunction"))
                            {
                                ScalingFunction v = (ScalingFunction) (int) Value[L"ScalingFunction"];

                                if (ScalingFunction::Linear <= v && v <= ScalingFunction::Period)
                                    _Configuration._ScalingFunction = v;
                            }

                            if (Value.Contains(L"ScalingFunctionFactor"))
                            {
                                double v = Value[L"ScalingFunctionFactor"];

                                if (0.0 <= v && v <= 1.0)
                                    _Configuration._SkewFactor = v;
                            }

                            if (Value.Contains(L"Bandwidth"))
                            {
                                double v = Value[L"Bandwidth"];

                                if (0.0 <= v && v <= 64.0)
                                    _Configuration._Bandwidth = v;
                            }

                            if (Value.Contains(L"NumberOfBands"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"NumberOfBands"];

                                if (2 <= v && v <= 512)
                                    _Configuration._NumBands = v;
                            }

                            if (Value.Contains(L"MinFrequency"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MinFrequency"];

                                if (v <= 96000)
                                    _Configuration._MinFrequency = v;
                            }

                            if (Value.Contains(L"MaxFrequency"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MaxFrequency"];

                                if (v <= 96000)
                                    _Configuration._MaxFrequency = v;
                            }

                            if (Value.Contains(L"_BandsPerOctave"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"_BandsPerOctave"];

                                if (1 <= v && v <= 48)
                                    _Configuration._BandsPerOctave = v;
                            }

                            if (Value.Contains(L"_MinNote"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"_MinNote"];

                                if (v <= 143)
                                    _Configuration._MinNote = v;
                            }

                            if (Value.Contains(L"_MaxNote"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"_MaxNote"];

                                if (v <= 143)
                                    _Configuration._MaxNote = v;
                            }

                            if (Value.Contains(L"Detune"))
                            {
                                int v = (int) Value[L"Detune"];

                                if (-24 <= v && v <= 24)
                                    _Configuration._Transpose= v;
                            }

                            if (Value.Contains(L"Pitch"))
                            {
                                double v = (double) Value[L"Pitch"];

                                if (0.0 <= v && v <= 96000)
                                    _Configuration._Pitch = v;
                            }

                            if (Value.Contains(L"SummationMethod"))
                            {
                                SummationMethod v = (SummationMethod) (int) Value[L"SummationMethod"];

                                if (SummationMethod::Minimum <= v && v <= SummationMethod::Median)
                                    _Configuration._SummationMethod = v;
                            }

                            if (Value.Contains(L"SmoothingMethod"))
                            {
                                SmoothingMethod v = (SmoothingMethod) (int) Value[L"SmoothingMethod"];

                                if (SmoothingMethod::Average <= v && v <= SmoothingMethod::Peak)
                                    _Configuration._SmoothingMethod = v;
                            }

                            if (Value.Contains(L"SmoothingFactor"))
                            {
                                double v = Value[L"SmoothingFactor"];

                                if (0.0 <= v && v <= 1.0)
                                    _Configuration._SmoothingFactor = v;
                            }

                            if (Value.Contains(L"XAxisMode"))
                            {
                                XAxisMode v = (XAxisMode) (int) Value[L"XAxisMode"];

                                if (XAxisMode::Bands <= v && v <= XAxisMode::Notes)
                                    _Configuration._XAxisMode = v;
                            }

                            if (Value.Contains(L"YAxisMode"))
                            {
                                YAxisMode v = (YAxisMode) (int) Value[L"YAxisMode"];

                                if (YAxisMode::Decibels <= v && v <= YAxisMode::Logarithmic)
                                    _Configuration._YAxisMode = v;
                            }

                            if (Value.Contains(L"BackgroundColor"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"BackgroundColor"]; // FIXME

//                              _Configuration._BackgroundColor = v;
                            }

                            if (Value.Contains(L"ColorScheme"))
                            {
                                ColorScheme v = (ColorScheme) (int) Value[L"ColorScheme"];

                                if (ColorScheme::Solid <= v && v <= ColorScheme::foobar2000DarkMode)
                                    _Configuration._ColorScheme = v;
                            }

                            if (Value.Contains(L"PeakMode"))
                            {
                                PeakMode v = (PeakMode) (int) Value[L"PeakMode"];

                                if (PeakMode::Classic <= v && v <= PeakMode::FadeOut)
                                    _Configuration._PeakMode = v;
                            }

                            if (Value.Contains(L"LogLevel"))
                            {
                                LogLevel v = (LogLevel) (int) Value[L"LogLevel"];

                                if (LogLevel::Trace <= v && v <= LogLevel::None)
                                    _Configuration._LogLevel = v;
                            }

                            Success = Reader.Read(Value);
                        }
                    }
                    catch (JSON::ReaderException e)
                    {
                        Log(LogLevel::Error, "%s: JSON Error: %s at position %d.", core_api::get_my_file_name(), ::utf8FromWide(e.GetMessage().c_str()).c_str(), e.GetPosition());
                    }

                    delete[] Text;
                }
            }

            delete[] UTF8;

            ::fclose(fp);
        }
    }
*/
}

Configuration _Configuration;

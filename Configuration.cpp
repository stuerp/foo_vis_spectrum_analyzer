
/** $VER: Configuration.cpp (2023.11.17) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"

#include "Configuration.h"
#include "Support.h"
#include "Resources.h"

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

Configuration _Configuration;

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
    _OptionsRect = {  };

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

    _SmoothingMethod = SmoothingMethod::Average;
    _SmoothingFactor = 0.5;                         // Smoothing constant, 0.0 .. 1.0

    _KernelSize = 32;                               // Lanczos interpolation kernel size, 1 .. 64
    _SummationMethod = SummationMethod::Maximum;    // Band power summation method
    _SmoothLowerFrequencies = true;
    _SmoothGainTransition = true;

    // Rendering parameters
    _BackgroundColor = D2D1::ColorF(0.f, 0.f, 0.f, 1.f);

    // X axis
    _XAxisMode = XAxisMode::Decades;

    // Y axis
    _YAxisMode = YAxisMode::Decibels;

    _MinDecibel = -90.;
    _MaxDecibel =   0.;

    _UseAbsolute = true;                               // Use absolute value
    _Gamma = 1.;

    // Band
    _ColorScheme = ColorScheme::Solid;
    _PeakMode = PeakMode::Classic;
    _DrawBandBackground = false;

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
            case 2:
            {
                parser >> _OptionsRect.left;
                parser >> _OptionsRect.top;
                parser >> _OptionsRect.right;
                parser >> _OptionsRect.bottom;

                parser >> _RefreshRateLimit; _RefreshRateLimit = pfc::clip_t<size_t>(_RefreshRateLimit, 20, 200);

                parser >> _UseHardwareRendering;
                parser >> _UseAntialiasing;

                parser >> _UseZeroTrigger;

                parser >> _WindowDuration; _WindowDuration = pfc::clip_t<size_t>(_WindowDuration, 50, 800);
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

    builder << _OptionsRect.left;
    builder << _OptionsRect.top;
    builder << _OptionsRect.right;
    builder << _OptionsRect.bottom;

    builder << _RefreshRateLimit;

    builder << _UseHardwareRendering;
    builder << _UseAntialiasing;

    builder << _UseZeroTrigger;
    builder << _WindowDuration;
}

void Configuration::Read()
{
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
}

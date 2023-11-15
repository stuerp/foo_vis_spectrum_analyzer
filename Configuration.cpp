
/** $VER: Configuration.cpp (2023.11.13) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"

#include "Configuration.h"
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

    _FFTSize = FFTSize::Fft4096;
    FrequencyDistribution = FrequencyDistributions::Octaves;

    // Common
    NumBands =    320;  // Number of frequency bands, 2 .. 512
    MinFrequency =    20;  // Hz, 0 .. 96000
    MaxFrequency = 20000;  // Hz, 0 .. 96000
    // Octaves
    BandsPerOctave =  12;    // Bands per octave, 1 .. 48
    MinNote =   0;    // Minimum note, 0 .. 143, 12 octaves
    MaxNote = 143;    // Maximum note, 0 .. 143, 12 octaves
    Detune =    0;    // Detune, -24 ..24
    _Pitch    = 440.0;   // Hz, 0 .. 96000, Octave bands tuning (nearest note = tuning frequency in Hz)

    // Frequencies
    ScalingFunction = ScalingFunctions::Logarithmic;

    SkewFactor = 0.0;   // Hz linear factor, 0.0 .. 1.0
    Bandwidth = 0.5;        // Bandwidth, 0.0 .. 64.0

    _SmoothingMethod = SmoothingMethod::Average;    // Smoothing method
    SmoothingFactor = 0.5;                       // Smoothing constant, 0.0 .. 1.0

    interpSize = 32;                                    // Lanczos interpolation kernel size, 1 .. 64
    _SummationMethod = SummationMethod::Maximum;  // Band power summation method
    smoothInterp = true;                               // Smoother bin interpolation on lower frequencies
    smoothSlope = true;                                // Smoother frequency slope on sum modes

    // ascale() Amplitude Scale
    UseDecibels = true;                               // Use decibel scale or logaritmic amplitude

    MinDecibels = -90.;                             // Lower amplitude, -120.0 .. 0.0
    MaxDecibels =   0.;                             // Upper amplitude, -120.0 .. 0.0

    _UseAbsolute = true;                               // Use absolute value

    Gamma = 1.;                                     // Gamma, 0.5 .. 10

    _XAxisMode = XAxisMode::Decades;

    _ColorScheme = ColorScheme::Solid;
    _PeakMode = PeakMode::Classic;

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
                console::printf("%s: Unknown configuration format. Version: %d", core_api::get_my_file_name(), Version);
        }
    }
    catch (exception_io & ex)
    {
        console::printf("%s: Exception while reading configuration data: %s", core_api::get_my_file_name(), ex.what());
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
                                FFTSize v = (FFTSize) (int) Value[L"FFTSize"];

                                if (v >= Fft64 && v <= Fft32768)
                                    _Configuration._FFTSize = v;
                            }

                            if (Value.Contains(L"FrequencyDistribution"))
                            {
                                FrequencyDistributions v = (FrequencyDistributions) (int) Value[L"FrequencyDistribution"];

                                if (v >= FrequencyDistributions::Frequencies && v <= FrequencyDistributions::AveePlayer)
                                    _Configuration.FrequencyDistribution = v;
                            }

                            if (Value.Contains(L"ScalingFunction"))
                            {
                                ScalingFunctions v = (ScalingFunctions) (int) Value[L"ScalingFunction"];

                                if (ScalingFunctions::Linear <= v && v <= ScalingFunctions::Period)
                                    _Configuration.ScalingFunction = v;
                            }

                            if (Value.Contains(L"ScalingFunctionFactor"))
                            {
                                double v = Value[L"ScalingFunctionFactor"];

                                if (0.0 <= v && v <= 1.0)
                                    _Configuration.SkewFactor = v;
                            }

                            if (Value.Contains(L"Bandwidth"))
                            {
                                double v = Value[L"Bandwidth"];

                                if (0.0 <= v && v <= 64.0)
                                    _Configuration.Bandwidth = v;
                            }

                            if (Value.Contains(L"NumberOfBands"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"NumberOfBands"];

                                if (2 <= v && v <= 512)
                                    _Configuration.NumBands = v;
                            }

                            if (Value.Contains(L"MinFrequency"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MinFrequency"];

                                if (v <= 96000)
                                    _Configuration.MinFrequency = v;
                            }

                            if (Value.Contains(L"MaxFrequency"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MaxFrequency"];

                                if (v <= 96000)
                                    _Configuration.MaxFrequency = v;
                            }

                            if (Value.Contains(L"BandsPerOctave"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"BandsPerOctave"];

                                if (1 <= v && v <= 48)
                                    _Configuration.BandsPerOctave = v;
                            }

                            if (Value.Contains(L"MinNote"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MinNote"];

                                if (v <= 143)
                                    _Configuration.MinNote = v;
                            }

                            if (Value.Contains(L"MaxNote"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MaxNote"];

                                if (v <= 143)
                                    _Configuration.MaxNote = v;
                            }

                            if (Value.Contains(L"Detune"))
                            {
                                int v = (int) Value[L"Detune"];

                                if (-24 <= v && v <= 24)
                                    _Configuration.Detune= v;
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
                                    _Configuration.SmoothingFactor = v;
                            }

                            if (Value.Contains(L"XAxisMode"))
                            {
                                XAxisMode v = (XAxisMode) (int) Value[L"XAxisMode"];

                                if (XAxisMode::Bands <= v && v <= XAxisMode::Notes)
                                    _Configuration._XAxisMode = v;
                            }

                            if (Value.Contains(L"PeakMode"))
                            {
                                PeakMode v = (PeakMode) (int) Value[L"PeakMode"];

                                if (PeakMode::Classic <= v && v <= PeakMode::AIMP)
                                    _Configuration._PeakMode = v;
                            }

                            if (Value.Contains(L"ColorScheme"))
                            {
                                ColorScheme v = (ColorScheme) (int) Value[L"ColorScheme"];

                                if (ColorScheme::Solid <= v && v <= ColorScheme::foobar2000DarkMode)
                                    _Configuration._ColorScheme = v;
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
                        console::printf("%s: JSON Error: %s at position %d.", core_api::get_my_file_name(), ::utf8FromWide(e.GetMessage().c_str()).c_str(), e.GetPosition());
                    }

                    delete[] Text;
                }
            }

            delete[] UTF8;

            ::fclose(fp);
        }
    }
}

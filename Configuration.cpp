
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
    _FrequencyDistribution = FrequencyDistribution::Octaves;

    // Common
    _numBands =    320;  // Number of frequency bands, 2 .. 512
    _minFreq =    20;  // Hz, 0 .. 96000
    _maxFreq = 20000;  // Hz, 0 .. 96000
    // Octaves
    _octaves =  12;    // Bands per octave, 1 .. 48
    _minNote =   0;    // Minimum note, 0 .. 143, 12 octaves
    _maxNote = 143;    // Maximum note, 0 .. 143, 12 octaves
    _detune =    0;    // Detune, -24 ..24
    _Pitch    = 440.0;   // Hz, 0 .. 96000, Octave bands tuning (nearest note = tuning frequency in Hz)

    // Frequencies
    _fscale = Logarithmic;

    _hzLinearFactor = 0.0;   // Hz linear factor, 0.0 .. 1.0
    _bandwidth = 0.5;        // Bandwidth, 0.0 .. 64.0

    _SmoothingMethod = TimeSmootingMethod::MethodAverage;    // Time smoothing method
    _SmoothingConstant = 0.0;                                            // Time smoothing constant, 0.0 .. 1.0

    interpSize = 32;                                    // Lanczos interpolation kernel size, 1 .. 64
    _SummationMode = SummationMode::Maximum;  // Band power summation method
    smoothInterp = true;                               // Smoother bin interpolation on lower frequencies
    smoothSlope = true;                                // Smoother frequency slope on sum modes

    // ascale() Amplitude Scale
    _UseDecibels = true;                               // Use decibel scale or logaritmic amplitude

    _MinDecibels = -90.;                             // Lower amplitude, -120.0 .. 0.0
    _MaxDecibels =   0.;                             // Upper amplitude, -120.0 .. 0.0

    _UseAbsolute = true;                               // Use absolute value

    _gamma = 1.;                                     // Gamma, 0.5 .. 10
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
    labelMode : 'decade',
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

                _RefreshRateLimit = pfc::clip_t<size_t>(_RefreshRateLimit, 20, 200);

                parser >> _UseHardwareRendering;
                parser >> _UseAntialiasing;

                parser >> _UseZeroTrigger;

                parser >> _WindowDuration;
                _WindowDuration = pfc::clip_t<size_t>(_WindowDuration, 50, 800);
                break;
            }

            default:
                FB2K_console_formatter() << core_api::get_my_file_name() << ": Unknown configuration format. Version: " << Version;
        }
    }
    catch (exception_io & ex)
    {
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Exception while reading configuration data: " << ex;
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
                                FrequencyDistribution v = (FrequencyDistribution) (int) Value[L"FrequencyDistribution"];

                                if (v >= FrequencyDistribution::Frequencies && v <= FrequencyDistribution::AveePlayer)
                                    _Configuration._FrequencyDistribution = v;
                            }

                            if (Value.Contains(L"ScalingFunction"))
                            {
                                ScalingFunctions v = (ScalingFunctions) (int) Value[L"ScalingFunction"];

                                if (ScalingFunctions::Linear <= v && v <= ScalingFunctions::Period)
                                    _Configuration._fscale = v;
                            }

                            if (Value.Contains(L"NumberOfBands"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"NumberOfBands"];

                                if (2 <= v && v <= 512)
                                    _Configuration._numBands = v;
                            }

                            if (Value.Contains(L"MinFrequency"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MinFrequency"];

                                if (v <= 96000)
                                    _Configuration._minFreq = v;
                            }

                            if (Value.Contains(L"MaxFrequency"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MaxFrequency"];

                                if (v <= 96000)
                                    _Configuration._maxFreq = v;
                            }

                            if (Value.Contains(L"BandsPerOctave"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"BandsPerOctave"];

                                if (1 <= v && v <= 48)
                                    _Configuration._octaves = v;
                            }

                            if (Value.Contains(L"MinNote"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MinNote"];

                                if (v <= 143)
                                    _Configuration._minNote = v;
                            }

                            if (Value.Contains(L"MaxNote"))
                            {
                                uint32_t v = (uint32_t) (int) Value[L"MaxNote"];

                                if (v <= 143)
                                    _Configuration._maxNote = v;
                            }

                            if (Value.Contains(L"Detune"))
                            {
                                int v = (int) Value[L"Detune"];

                                if (-24 <= v && v <= 24)
                                    _Configuration._detune= v;
                            }

                            if (Value.Contains(L"Pitch"))
                            {
                                double v = (double) Value[L"Pitch"];

                                if (0.0 <= v && v <= 96000)
                                    _Configuration._Pitch = v;
                            }

                            Success = Reader.Read(Value);
                        }
                    }
                    catch (JSON::ReaderException e)
                    {
                        FB2K_console_formatter() << "Error: " << e.GetMessage().c_str() << " (Position " << e.GetPosition() << ")";
                    }

                    delete[] Text;
                }
            }

            delete[] UTF8;

            ::fclose(fp);
        }
    }
}

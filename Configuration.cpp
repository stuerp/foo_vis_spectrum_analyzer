
/** $VER: Configuration.cpp (2022.11.11) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Resources.h"
#include "Configuration.h"

#pragma hdrstop

size_t Config::GetVersion()
{
    return NUM_CONFIG_VERSION;
}

Config::Config()
{
    Reset();
}

void Config::Reset()
{
    _UseHardwareRendering = true;

    _DoDownMixing = false;
    _UseZeroTrigger = false;
    _DoResampling = false;
    _UseLowQuality = false;
    _WindowDuration = 100;
    _ZoomFactor = 100;
    _RefreshRateLimit = 20;
    _LineStrokeWidth = 10;

}

void Config::Build(ui_element_config_builder & builder)
{
    builder << GetVersion();
    builder << _LineStrokeWidth;
    builder << _UseLowQuality;
    builder << _DoResampling;
    builder << _RefreshRateLimit;
    builder << _UseZeroTrigger;
    builder << _UseHardwareRendering;
    builder << _DoDownMixing;
    builder << _WindowDuration;
    builder << _ZoomFactor;
}

void Config::Parse(ui_element_config_parser & parser)
{
    Reset();

    try
    {
        size_t Version;

        parser >> Version;

        switch (Version)
        {
            case 2:
                // Fallthrough

            case 1:
                parser >> _UseHardwareRendering;
                parser >> _DoDownMixing;
                parser >> _WindowDuration;
                _WindowDuration = pfc::clip_t<size_t>(_WindowDuration, 50, 800);

                parser >> _ZoomFactor;
                _ZoomFactor = pfc::clip_t<size_t>(_ZoomFactor, 50, 800);

                parser >> _UseZeroTrigger;

                parser >> _RefreshRateLimit;
                _RefreshRateLimit = pfc::clip_t<size_t>(_RefreshRateLimit, 20, 200);

                parser >> _DoResampling;
                parser >> _UseLowQuality;

                parser >> _LineStrokeWidth;
                _LineStrokeWidth = pfc::clip_t<size_t>(_LineStrokeWidth, 5, 30);
                break;

            default:
                FB2K_console_formatter() << core_api::get_my_file_name() << ": Unknown configuration format. Version: " << Version;
        }
    }
    catch (exception_io & ex)
    {
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Exception while reading configuration data: " << ex;
    }
}

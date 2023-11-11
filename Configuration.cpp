
/** $VER: Configuration.cpp (2023.11.11) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Resources.h"
#include "Configuration.h"

#pragma hdrstop

size_t Configuration::GetVersion()
{
    return NUM_CONFIG_VERSION;
}

Configuration::Configuration()
{
    Reset();
}

void Configuration::Reset()
{
    _RefreshRateLimit = 20;

    _UseHardwareRendering = true;
    _UseAntialiasing = true;

    _UseZeroTrigger = false;
    _WindowDuration = 100;
}

void Configuration::Build(ui_element_config_builder & builder)
{
    builder << GetVersion();

    builder << _RefreshRateLimit;

    builder << _UseHardwareRendering;
    builder << _UseAntialiasing;

    builder << _UseZeroTrigger;
    builder << _WindowDuration;
}

void Configuration::Parse(ui_element_config_parser & parser)
{
    Reset();

    try
    {
        size_t Version;

        parser >> Version;

        switch (Version)
        {
            case 1:
                parser >> _RefreshRateLimit;
                _RefreshRateLimit = pfc::clip_t<size_t>(_RefreshRateLimit, 20, 200);

                parser >> _UseHardwareRendering;
                parser >> _UseAntialiasing;

                parser >> _UseZeroTrigger;

                parser >> _WindowDuration;
                _WindowDuration = pfc::clip_t<size_t>(_WindowDuration, 50, 800);
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

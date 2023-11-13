
/** $VER: Configuration.cpp (2023.11.13) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Configuration.h"
#include "Resources.h"

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
    _OptionsRect = {  };

    _RefreshRateLimit = 20;

    _UseHardwareRendering = true;
    _UseAntialiasing = true;

    _UseZeroTrigger = false;
    _WindowDuration = 100;
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

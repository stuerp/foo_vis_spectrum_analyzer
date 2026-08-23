
/** $VER: Component.cpp (2025.10.20) P. Stuer **/

#include "pch.h"

#include <sdk/componentversion.h>

#include "State.h"
#include "Resources.h"
#include "Log.h"

#include "Support.h"

#pragma hdrstop

namespace
{
    #pragma warning(disable: 4265 5026 5027 26433 26436 26455)
    DECLARE_COMPONENT_VERSION
    (
        STR_COMPONENT_NAME,
        STR_COMPONENT_VERSION,
        STR_COMPONENT_BASENAME " " STR_COMPONENT_VERSION "\n"
            STR_COMPONENT_COPYRIGHT "\n"
            "\n"
            STR_COMPONENT_DESCRIPTION "\n"
            STR_COMPONENT_COMMENT "\n"
            "\n"
            "Built with foobar2000 SDK " TOSTRING(FOOBAR2000_SDK_VERSION) "\n"
            "on " __DATE__ " " __TIME__ "."
    )
    VALIDATE_COMPONENT_FILENAME(STR_COMPONENT_FILENAME)
}

class Component : public init_stage_callback
{
public:
    void on_init_stage(t_uint32 stage) noexcept override
    {
        if (stage == init_stages::after_config_read)
        {
            Log.SetLevel((LogLevel) CfgLogLevel.get());
        }
        else
        if (stage == init_stages::before_ui_init)
        {
            InitializeDpiAwareness();
        }
    }
};

static initquit_factory_t<Component> _Component;


/** $VER: Component.cpp (2024.01.05) P. Stuer **/

#include "framework.h"

#include <sdk/componentversion.h>

#include "Resources.h"

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

class Component : public initquit
{
public:
    virtual void FB2KAPI on_init()
    {
        if (core_api::is_quiet_mode_enabled())
            return;
    }

    virtual void FB2KAPI on_quit()
    {
    }
};

static initquit_factory_t<Component> _Component;

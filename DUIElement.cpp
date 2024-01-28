
/** $VER: DUIElement.cpp (2024.01.28) P. Stuer **/

#include "DUIElement.h"

#include "Log.h"

#pragma hdrstop

#pragma region ui_element_instance interface

/// <summary>
/// Initializes a new instance.
/// </summary>
DUIElement::DUIElement(ui_element_config::ptr data, ui_element_instance_callback::ptr callback) : m_callback(callback)
{
    _Configuration._DefBackColor = m_callback->query_std_color(ui_color_background);
    _Configuration._DefTextColor = m_callback->query_std_color(ui_color_text);

    set_configuration(data);
}

/// <summary>
/// Retrieves the name of the element.
/// </summary>
void DUIElement::g_get_name(pfc::string_base & p_out)
{
    p_out = STR_COMPONENT_NAME;
}

/// <summary>
/// Retrieves the description of the element.
/// </summary>
const char * DUIElement::g_get_description()
{
    return "Spectum analyzer visualization using DirectX";
}

/// <summary>
/// Retrieves the GUID of the element.
/// </summary>
GUID DUIElement::g_get_guid()
{
    return UIElement::GetGUID();
}

/// <summary>
/// Retrieves the subclass GUID of the element.
/// </summary>
GUID DUIElement::g_get_subclass()
{
    return ui_element_subclass_playback_visualisation;
}

/// <summary>
/// Retrieves the default configuration of the element.
/// </summary>
ui_element_config::ptr DUIElement::g_get_default_configuration()
{
    ui_element_config_builder Builder;

    try
    {
        Configuration DefaultConfiguration;

        DefaultConfiguration.Write(Builder);
    }
    catch (exception_io & ex)
    {
        Log::Write(Log::Level::Error, "%s: Exception while writing DUI default configuration data: %s", core_api::get_my_file_name(), ex.what());
    }

    return Builder.finish(g_get_guid());
}

/// <summary>
/// Initializes the element's windows.
/// </summary>
void DUIElement::initialize_window(HWND p_parent)
{
    const DWORD Style = 0;
    const DWORD ExStyle = 0;

    this->Create(p_parent, nullptr, nullptr, Style, ExStyle);
}

/// <summary>
/// Alters element's current configuration. Specified ui_element_config's GUID must be the same as this element's GUID.
/// </summary>
void DUIElement::set_configuration(ui_element_config::ptr data)
{
    ui_element_config_parser Parser(data);

    _Configuration.Read(Parser);
}

/// <summary>
/// Retrieves element's current configuration. Returned object's GUID must be set to your element's GUID so your element can be re-instantiated with stored settings.
/// </summary>
ui_element_config::ptr DUIElement::get_configuration()
{
    ui_element_config_builder Builder;

    _Configuration.Write(Builder);

    return Builder.finish(g_get_guid());
}

/// <summary>
/// Used by host to notify the element about various events. See ui_element_notify_* GUIDs for possible p_what parameter; meaning of other parameters depends on p_what value. Container classes should dispatch all notifications to their children.
/// </summary>
void DUIElement::notify(const GUID & what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
    if (what == ui_element_notify_colors_changed)
    {
        _Configuration._DefBackColor = m_callback->query_std_color(ui_color_background);
        _Configuration._DefTextColor = m_callback->query_std_color(ui_color_text);

        Invalidate();
    }
}

static service_factory_single_t<ui_element_impl_visualisation<DUIElement>> _Factory;

#pragma endregion

/// <summary>
/// Handles a context menu selection.
/// </summary>
void DUIElement::OnContextMenu(CWindow wnd, CPoint position)
{
    if (m_callback->is_edit_mode_enabled())
        SetMsgHandled(FALSE);
    else
        UIElement::OnContextMenu(wnd, position);
}

/// <summary>
/// Toggles full screen mode.
/// </summary>
void DUIElement::ToggleFullScreen() noexcept
{
    static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
    _IsFullScreen != _IsFullScreen;
}

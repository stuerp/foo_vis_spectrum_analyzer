
/** $VER: DUIElement.cpp (2026.03.21) P. Stuer **/

#include "pch.h"

#include "DUIElement.h"
#include "Color.h"
#include "Log.h"

#pragma hdrstop

#pragma region ui_element_instance interface

/// <summary>
/// Initializes a new instance.
/// </summary>
dui_element_t::dui_element_t(ui_element_config::ptr data, ui_element_instance_callback::ptr callback) : m_callback(callback)
{
    _UIState._IsDUI = true;

    GetColors();

    set_configuration(data);
}

/// <summary>
/// Retrieves the name of the element.
/// </summary>
void dui_element_t::g_get_name(pfc::string_base & name)
{
    name = STR_COMPONENT_NAME;
}

/// <summary>
/// Retrieves the description of the element.
/// </summary>
const char * dui_element_t::g_get_description()
{
    return "Spectum analyzer visualization using DirectX";
}

/// <summary>
/// Retrieves the GUID of the element.
/// </summary>
GUID dui_element_t::g_get_guid()
{
    return uielement_t::GetGUID();
}

/// <summary>
/// Retrieves the subclass GUID of the element.
/// </summary>
GUID dui_element_t::g_get_subclass()
{
    return ui_element_subclass_playback_visualisation;
}

/// <summary>
/// Initializes the element's windows.
/// </summary>
void dui_element_t::initialize_window(HWND p_parent)
{
    const DWORD Style = 0;
    const DWORD ExStyle = 0;

    this->Create(p_parent, nullptr, nullptr, Style, ExStyle);
}

/// <summary>
/// Sets the element's current configuration.
/// </summary>
void dui_element_t::set_configuration(ui_element_config::ptr configData)
{
    // Try to read the data as JSON first. (v0.11.0.0 and later)
    try
    {
        _UIState.FromJSON((const char *) configData->get_data(), configData->get_data_size());
    }
    catch (...)
    {
        // Try to read the data as a binary stream. (Legacy)
        ui_element_config_parser Parser(configData);

        _UIState.Read(&Parser.m_stream, Parser.get_remaining());
    }
}

/// <summary>
/// Gets the element's default configuration.
/// </summary>
ui_element_config::ptr dui_element_t::g_get_default_configuration()
{
    state_t DefaultConfiguration;

    return get_configuration(DefaultConfiguration);
}

/// <summary>
/// Gets the element's current configuration.
/// </summary>
ui_element_config::ptr dui_element_t::get_configuration()
{
    return get_configuration(_UIState);
}

/// <summary>
/// Gets the element's configuration.
/// </summary>
ui_element_config::ptr dui_element_t::get_configuration(state_t & state)
{
    // Try to write the data as JSON first. (v0.11.0.0 and later)
    try
    {
        std::string Config = state.ToJSON(false).dump(-1);

        return ui_element_config::g_create(g_get_guid(), Config.c_str(), Config.size());
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write("Failed to serialize configuration to JSON, falling back to binary stream. Error: ", e.what());

        // Try to write the data as a binary stream. (Legacy)
        ui_element_config_builder Builder;

        state.Write(&Builder.m_stream);

        return Builder.finish(g_get_guid());
    }
}

/// <summary>
/// Used by the host to notify the element about various events.
/// See ui_element_notify_* GUIDs for possible p_what parameter; meaning of other parameters depends on p_what value.
/// Container classes should dispatch all notifications to their children.
/// </summary>
void dui_element_t::notify(const GUID & what, t_size param1, const void * param2, t_size param2Size)
{
    if (what == ui_element_notify_colors_changed)
    {
        OnColorsChanged();
    }
    else
    if (what == ui_element_notify_font_changed)
    {
//      m_callback->query_font_ex(ui_font_default);
    }
    else
    if (what == ui_element_notify_visibility_changed)
    {
        _IsVisible = (bool) param1;
    }
}

static service_factory_single_t<ui_element_impl_visualisation<dui_element_t>> _Factory;

#pragma endregion
/*
/// <summary>
/// Handles the WM_ERASEBKGND message.
/// </summary>
LRESULT dui_element_t::OnEraseBackground(CDCHandle hDC)
{
    if (!_IsInitializing)
        return 0;

    RECT cr;

    GetClientRect(&cr);

    HBRUSH hBrush = color_t::CreateBrush(_UIThread._StyleManager.UserInterfaceColors[1]);

    ::FillRect(hDC, &cr, hBrush);

    ::DeleteObject((HGDIOBJ) hBrush);

    _IsInitializing = false;

    return 1; // Prevent GDI from erasing the background. Required for transparency.
}
*/
/// <summary>
/// Handles a context menu selection.
/// </summary>
void dui_element_t::OnContextMenu(CWindow wnd, CPoint position)
{
    if (m_callback->is_edit_mode_enabled())
        SetMsgHandled(FALSE);
    else
        uielement_t::OnContextMenu(wnd, position);
}

/// <summary>
/// Toggles full screen mode.
/// </summary>
void dui_element_t::ToggleFullScreen() noexcept
{
    static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
}

/// <summary>
/// Gets the user interface colors.
/// </summary>
void dui_element_t::GetColors() noexcept
{
    _UIState._StyleManager.UserInterfaceColors.clear();

    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(m_callback->query_std_color(ui_color_text)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(m_callback->query_std_color(ui_color_background)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(m_callback->query_std_color(ui_color_highlight)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(m_callback->query_std_color(ui_color_selection)));
    _UIState._StyleManager.UserInterfaceColors.push_back(color_t::ToD2D1_COLOR_F(m_callback->query_std_color(ui_color_darkmode)));
}

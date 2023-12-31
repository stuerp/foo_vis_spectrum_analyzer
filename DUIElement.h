
/** $VER: DUIElement.h (2023.12.06) P. Stuer - Default User Interface support **/

#pragma once

#include "framework.h"

#include <helpers/BumpableElem.h>

#include "UIElement.h"

/// <summary>
/// Implements a Default UI element.
/// </summary>
class DUIElement : public ui_element_instance, public UIElement
{
public:
    DUIElement(ui_element_config::ptr data, ui_element_instance_callback::ptr callback);

    DUIElement(const DUIElement &) = delete;
    DUIElement & operator=(const DUIElement &) = delete;
    DUIElement(DUIElement &&) = delete;
    DUIElement & operator=(DUIElement &&) = delete;

    void OnContextMenu(CWindow wnd, CPoint position) override final;

    /// <summary>
    /// Toggles full screen mode.
    /// </summary>
    void ToggleFullScreen() noexcept override
    {
        static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
    }

    // Default User Interface
    #pragma region ui_element_instance interface
    static void g_get_name(pfc::string_base & p_out);
    static const char * g_get_description();
    static GUID g_get_guid();
    static GUID g_get_subclass();
    static ui_element_config::ptr g_get_default_configuration();

    void initialize_window(HWND p_parent);
    virtual void set_configuration(ui_element_config::ptr p_data);
    virtual ui_element_config::ptr get_configuration();
    virtual void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);
    #pragma endregion

protected:
    ui_element_instance_callback::ptr m_callback;
};


/** $VER: DUIElement.h (2026.03.21) P. Stuer - Default User Interface support **/

#pragma once

#include "pch.h"

#include <helpers\BumpableElem.h>

#include "UIElement.h"

/// <summary>
/// Implements a Default UI element.
/// </summary>
class dui_element_t : public uielement_t, public ui_element_instance
{
public:
    dui_element_t(ui_element_config::ptr data, ui_element_instance_callback::ptr callback);

    dui_element_t(const dui_element_t &) = delete;
    dui_element_t & operator=(const dui_element_t &) = delete;
    dui_element_t(dui_element_t &&) = delete;
    dui_element_t & operator=(dui_element_t &&) = delete;

//  LRESULT OnEraseBackground(CDCHandle hDC) override final;
    void OnContextMenu(CWindow wnd, CPoint position) override final;

    void ToggleFullScreen() noexcept override final;

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
    virtual void notify(const GUID & what, t_size param1, const void * param2, t_size param2Size);

    #pragma endregion

private:
    void GetColors() noexcept override;

    static ui_element_config::ptr get_configuration(state_t & state);

protected:
    ui_element_instance_callback::ptr m_callback;
};

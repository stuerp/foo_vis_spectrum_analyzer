
/** $VER: CUIElement.h (2024.01.19) P. Stuer - Columns User Interface support **/

#pragma once

#include "framework.h"

#include "UIElement.h"

#include <ui_extension.h>

namespace uie
{
/// <summary>
/// Implements a Columns UI element.
/// </summary>
class CUIElement : public UIElement, public window
{
public:
    CUIElement();

    CUIElement(const CUIElement &) = delete;
    CUIElement & operator=(const CUIElement &) = delete;
    CUIElement(CUIElement &&) = delete;
    CUIElement & operator=(CUIElement &&) = delete;

    #pragma region window interface

    /// <summary>
    /// Gets the category of the extension.
    /// </summary>
    void get_category(pfc::string_base & out) const final
    {
        out = "Visualisations";
    }

    /// <summary>
    /// Gets the type of the extension.
    /// </summary>
    uint32_t get_type() const final
    {
        return uie::type_panel;
    }

    HWND create_or_transfer_window(HWND parent, const window_host_ptr & newHost, const ui_helpers::window_position_t & position);

    virtual void destroy_window();

    /// <summary>
    /// Returns true if the extension is available.
    /// </summary>
    virtual bool is_available(const window_host_ptr & p) const
    {
        return true;
    }

    /// <summary>
    /// Gets the window handle of the extension.
    /// </summary>
    virtual HWND get_wnd() const
    {
        return *this;
    }

    #pragma endregion

    #pragma region container_uie_window_v3_t interface
/*
    const window_host_ptr & get_host() const
    {
        return _Host;
    }
*/
    #pragma endregion

    #pragma region extension_base interface

    /// <summary>
    /// Gets the unique ID of extension.
    /// </summary>
    const GUID & get_extension_guid() const
    {
        return GetGUID();
    }

    /// <summary>
    /// Gets a user-readable name of the extension.
    /// </summary>
    void get_name(pfc::string_base & out) const
    {
        out = STR_COMPONENT_NAME;
    }

    /// <summary>
    /// Sets an instance of the configuration data.
    /// </summary>
    void set_config(stream_reader * reader, size_t size, abort_callback & abortHandler) final
    {
        _Configuration.Read(reader, size, abortHandler);
    }

    /// <summary>
    /// Gets an instance of the configuration data.
    /// </summary>
    void get_config(stream_writer * writer, abort_callback & abortHandler) const final
    {
        _Configuration.Write(writer, abortHandler);
    }

    #pragma endregion

private:
    window_host_ptr _Host;
};
}


/** $VER: CUIElement.h (2024.03.13) P. Stuer - Columns User Interface support **/

#pragma once

#include "pch.h"

#include "UIElement.h"

#include <ui_extension.h>

namespace uie
{
class CUIColorClient;

/// <summary>
/// Implements a Columns UI element.
/// </summary>
class CUIElement : public uielement_t, public uie::window
{
public:
    CUIElement();

    CUIElement(const CUIElement &) = delete;
    CUIElement & operator=(const CUIElement &) = delete;
    CUIElement(CUIElement &&) = delete;
    CUIElement & operator=(CUIElement &&) = delete;

    virtual ~CUIElement();

    LRESULT OnEraseBackground(CDCHandle hDC) override final;
    void ToggleFullScreen() noexcept override final;

    // Columns User Interface
    #pragma region uie::window interface

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
        _MainState.Read(reader, size, abortHandler);
    }

    /// <summary>
    /// Gets an instance of the configuration data.
    /// </summary>
    void get_config(stream_writer * writer, abort_callback & abortHandler) const final
    {
        _MainState.Write(writer, abortHandler);
    }

    #pragma endregion

private:
    void GetColors() noexcept override;

private:
    window_host_ptr _Host;
    HWND _hParent;
};

static std::vector<CUIElement *> _Elements; // Very ugly but necessary because of the weird CUI notification mechanism.

/// <summary>
/// Receives notifications from CUI when the colors change.
/// </summary>
class CUIColorClient : public cui::colours::client
{
public:
    CUIColorClient() { }

    CUIColorClient(const CUIColorClient &) = delete;
    CUIColorClient & operator=(const CUIColorClient &) = delete;
    CUIColorClient(CUIColorClient &&) = delete;
    CUIColorClient & operator=(CUIColorClient &&) = delete;

    virtual ~CUIColorClient() { }

    #pragma region cui::colours::client

    virtual const GUID & get_client_guid() const
    {
        static const GUID guid = GUID_UI_ELEMENT;

        return guid;
   }

    virtual void get_name(pfc::string_base & out) const
    {
        out = STR_COMPONENT_NAME;
    }

    /// <summary>
    /// Return a combination of bool_flag_t to indicate which boolean flags are supported. 
    /// </summary>
    virtual uint32_t get_supported_bools() const override
    {
        return cui::colours::bool_dark_mode_enabled;
    }

    /// <summary>
    /// Indicates whether the Theme API is supported.
    /// </summary>
    virtual bool get_themes_supported() const override
    {
        return false;
    }

    virtual void on_colour_changed(uint32_t changed_items_mask) const override;

    /// <summary>
    /// Called whenever a supported boolean flag changes. Support for a flag is determined using the get_supported_bools() method.
    /// </summary>
    virtual void on_bool_changed(uint32_t changed_items_mask) const override;

    #pragma endregion

    static void Register(CUIElement * element)
    {
        if (element == nullptr)
            return;

        _Elements.push_back(element);
    }

    static void Unregister(CUIElement * element)
    {
        auto Element = std::find(_Elements.begin(), _Elements.end(), element);

        if (Element != _Elements.end())
            _Elements.erase(Element);
    }

    #pragma endregion
};

}

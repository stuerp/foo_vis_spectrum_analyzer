
/** $VER: CUINotificationHandler.h (2026.03.22) P. Stuer - Handles CUI notifications **/

#pragma once

#include "pch.h"

#include "CUIElement.h"

namespace uie
{

/// <summary>
/// Handles CUI notifications.
/// </summary>
class cui_notification_handler_t : public cui::colours::client
{
public:
    cui_notification_handler_t() { }

    cui_notification_handler_t(const cui_notification_handler_t &) = delete;
    cui_notification_handler_t & operator=(const cui_notification_handler_t &) = delete;
    cui_notification_handler_t(cui_notification_handler_t &&) = delete;
    cui_notification_handler_t & operator=(cui_notification_handler_t &&) = delete;

    virtual ~cui_notification_handler_t() noexcept { }

    #pragma region cui::colours::client

    const GUID & get_client_guid() const override final
    {
        static const GUID guid = GUID_UI_ELEMENT;

        return guid;
   }

    void get_name(pfc::string_base & out) const override final
    {
        out = STR_COMPONENT_NAME;
    }

    /// <summary>
    /// Return a combination of bool_flag_t to indicate which boolean flags are supported. 
    /// </summary>
    uint32_t get_supported_bools() const override final
    {
        return cui::colours::bool_dark_mode_enabled;
    }

    /// <summary>
    /// Indicates whether the Theme API is supported.
    /// </summary>
    bool get_themes_supported() const override final
    {
        return false;
    }

    void on_colour_changed(uint32_t changed_items_mask) const override final;

    void on_bool_changed(uint32_t changed_items_mask) const override final;

    #pragma endregion

    static void Register(cui_element_t * element)
    {
        if (element == nullptr)
            return;

        _Elements.push_back(element);
    }

    static void Unregister(cui_element_t * element)
    {
        auto Element = std::find(_Elements.begin(), _Elements.end(), element);

        if (Element != _Elements.end())
            _Elements.erase(Element);
    }

    #pragma endregion

private:
    static std::vector<cui_element_t *> _Elements; // Very ugly but necessary because of the weird CUI notification mechanism.
};

}

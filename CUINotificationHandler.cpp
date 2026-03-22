
/** $VER: CUINotificationHandler.cpp (2026.03.22) P. Stuer **/

#include "pch.h"

#include "CUINotificationHandler.h"

#pragma hdrstop

namespace uie
{

/// <summary>
/// Handles color change notifications from CUI.
/// </summary>
void cui_notification_handler_t::on_colour_changed(uint32_t changed_items_mask) const
{
    for (auto Iter : _Elements)
        Iter->OnColorsChanged();
}

/// <summary>
/// Called whenever a supported boolean flag changes. Support for a flag is determined using the get_supported_bools() method.
/// </summary>
void cui_notification_handler_t::on_bool_changed(uint32_t changed_items_mask) const
{
}

std::vector<cui_element_t *> cui_notification_handler_t::_Elements; // Very ugly but necessary because of the weird CUI notification mechanism.

static cui::colours::client::factory<cui_notification_handler_t> _CUINotificationHandlerFactory;

}

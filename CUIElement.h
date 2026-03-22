
/** $VER: CUIElement.h (2026.03.21) P. Stuer - Columns User Interface support **/

#pragma once

#include "pch.h"

#include "UIElement.h"

#include "MemoryStream.h"

#include <ui_extension.h>

namespace uie
{
/// <summary>
/// Implements a Columns UI element.
/// </summary>
class cui_element_t : public uielement_t, public uie::window
{
public:
    cui_element_t();

    cui_element_t(const cui_element_t &) = delete;
    cui_element_t & operator=(const cui_element_t &) = delete;
    cui_element_t(cui_element_t &&) = delete;
    cui_element_t & operator=(cui_element_t &&) = delete;

    virtual ~cui_element_t() noexcept;

//  LRESULT OnEraseBackground(CDCHandle hDC) override final;
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
    void set_config(stream_reader * stream, size_t size, abort_callback & abortHandler) final
    {
        std::vector<uint8_t> Config(size);

        stream->read(Config.data(), size, abortHandler);

        // Try to read the data as JSON first. (v0.11.0.0 and later)
        try
        {
            _UIState.FromJSON((const char *) Config.data(), Config.size());
        }
        catch (...)
        {
            service_ptr_t<file> Stream;

            try
            {
                Stream = fb2k::service_new<memory_stream_t>(Config.data(), Config.size());

                _UIState.Read(Stream.get_ptr(), size, abortHandler);
            }
            catch (const exception_io & e)
            {
                Log.AtError().Write("Failed to deserialize configuration to binary stream. Error: ", e.what());
            }
        }
    }

    /// <summary>
    /// Gets an instance of the configuration data.
    /// </summary>
    void get_config(stream_writer * stream, abort_callback & abortHandler) const final
    {
        // Try to write the data as JSON first. (v0.11.0.0 and later)
        try
        {
            std::string Config = _UIState.ToJSON(false).dump(-1);

            stream->write_string_raw(Config.c_str(), abortHandler);
        }
        catch (const std::exception & e)
        {
            Log.AtError().Write("Failed to serialize configuration to JSON, falling back to binary stream. Error: ", e.what());

            // Try to write the data as a binary stream. (Legacy)
            _UIState.Write(stream, abortHandler);
        }
    }

    #pragma endregion

private:
    void GetColors() noexcept override;

private:
    window_host_ptr _Host;
    HWND _hParent;
};

}

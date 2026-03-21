
/** $VER: CUIElement.h (2026.03.21) P. Stuer - Columns User Interface support **/

#pragma once

#include "pch.h"

#include "UIElement.h"

#include <ui_extension.h>

#include <sdk\file.h>
#include <sdk\filesystem.h>
#include <sdk\exception_io.h>

#include <pfc\pfc.h>

using namespace foobar2000_io;

namespace uie
{
/// <summary>
/// Implements a memory stream.
/// </summary>
class memory_stream_t : public file
{
public:
    memory_stream_t(const void * data, t_size size) : _Data((const uint8_t *) data), _Size(size), _Position() { }

    memory_stream_t(const memory_stream_t &) = delete;
    memory_stream_t & operator=(const memory_stream_t &) = delete;
    memory_stream_t(memory_stream_t &&) = delete;
    memory_stream_t & operator=(memory_stream_t &&) = delete;

    virtual ~memory_stream_t() noexcept = default;

    #pragma region stream_reader interface

    t_size read(void * data, t_size size, abort_callback & abortHandler) override final
    {
        t_size Delta = pfc::min_t<t_size>(size, (t_size) (_Size - _Position));

        if (Delta > 0)
        {
            std::memcpy(data, _Data + _Position, Delta);

            _Position += Delta;
        }

        return Delta;
    }

    #pragma endregion

    #pragma region stream_writer interface

    void write(const void *, t_size, abort_callback &) override final
    {
        throw exception_io_denied_readonly();
    }

    #pragma endregion

    #pragma region file interface

    t_filesize get_size(abort_callback &) override final
    {
        return _Size;
    }

    t_filesize get_position(abort_callback &) override final
    {
        return _Position;
    }

    void resize(t_filesize, abort_callback &) override final
    {
        throw exception_io_denied_readonly();
    }

    void seek(t_filesize offset, abort_callback &) override final
    {
        if (offset > _Size)
            throw exception_io_seek_out_of_range();

        _Position = offset;
    }

    bool can_seek() override final
    {
        return true;
    }

    bool get_content_type(pfc::string_base & text) override final
    {
        text = "application/octet-stream";

        return true;
    }

    void reopen(abort_callback &) override final
    {
        _Position = 0;
    }

    bool is_remote() override final
    {
        return false;
    }

    #pragma endregion

private:
    const uint8_t * _Data;
    t_filesize _Size;
    t_filesize _Position;
};

class cui_color_client_t;

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

static std::vector<cui_element_t *> _Elements; // Very ugly but necessary because of the weird CUI notification mechanism.

/// <summary>
/// Receives notifications from CUI when the colors change.
/// </summary>
class cui_color_client_t : public cui::colours::client
{
public:
    cui_color_client_t() { }

    cui_color_client_t(const cui_color_client_t &) = delete;
    cui_color_client_t & operator=(const cui_color_client_t &) = delete;
    cui_color_client_t(cui_color_client_t &&) = delete;
    cui_color_client_t & operator=(cui_color_client_t &&) = delete;

    virtual ~cui_color_client_t() noexcept { }

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
};

}

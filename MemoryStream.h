
/** $VER: MemoryStream.h (2026.03.22) P. Stuer - Implements a memory stream using the foobar2000 API **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX

#include <sdk\foobar2000.h>

using namespace foobar2000_io;

/// <summary>
/// Implements a memory stream using the foobar2000 API.
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
        t_size Delta = std::min(size, (t_size) (_Size - _Position));

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

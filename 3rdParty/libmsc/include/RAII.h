
/** $VER: RAII.h (2026.06.10) P. Stuer - Various RAII wrappers for common OS resources **/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>

#include <filesystem>

namespace fs = std::filesystem;

#include "CriticalSection.h"

namespace msc
{

/// <summary>
/// Implements a safe HANDLE wrapper. 
/// </summary>
class handle_t
{
public:
    handle_t(HANDLE handle) noexcept : _Handle(handle)
    {
    }

    handle_t(const handle_t & other) = delete;
    handle_t & operator=(const handle_t & other) = delete;

    handle_t(handle_t && other) noexcept : _Handle(other._Handle)
    {
        other._Handle = INVALID_HANDLE_VALUE;
    }

    handle_t & operator=(handle_t && other) noexcept
    {
        if (this != &other)
        {
            Close();

            _Handle = other._Handle;
            other._Handle = INVALID_HANDLE_VALUE;
        }

        return *this;
    }

    ~handle_t() noexcept
    {
        Close();
    }

    operator HANDLE() const noexcept
    {
        return _Handle;
    }

    bool IsValid() const noexcept
    {
        return _Handle != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE _Handle = INVALID_HANDLE_VALUE;

    void Close() noexcept
    {
        if (_Handle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(_Handle);
            _Handle = INVALID_HANDLE_VALUE;
        }
    }
};

/// <summary>
/// Implements a synchronization lock. 
/// </summary>
class lock_t
{
public:
    lock_t(critical_section_t & cs) : _cs(cs)
    {
        _cs.Enter();
    }

    lock_t(const lock_t &) = delete;
    lock_t & operator=(const lock_t &) = delete;
    lock_t(lock_t &&) = delete;
    lock_t & operator=(lock_t &&) = delete;

    ~lock_t()
    {
        _cs.Leave();
    }
     
private:
    critical_section_t & _cs;
};

}

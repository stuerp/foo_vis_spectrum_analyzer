
/** $VER: error_t.h (2025.09.16) P. Stuer - Encapsulates the Win32 error number. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <windows.h>

#include <string>

class error_t
{
public:
    error_t() : _Number(NOERROR) { }

    error_t(const error_t &);
    error_t & operator=(const error_t &);
    error_t(error_t &&) = delete;
    error_t & operator=(error_t &&) = delete;

    virtual ~error_t() { }

    error_t(DWORD number) : _Number(number) { }
    error_t & operator=(DWORD);

    operator DWORD() const noexcept { return _Number; }

    std::wstring Message() const noexcept;

private:
    DWORD _Number;
};

inline error_t::error_t(const error_t & other)
{
    _Number= other._Number;
}

inline error_t & error_t::operator=(const error_t & other)
{
    _Number = other._Number;

    return *this;
}

inline error_t & error_t::operator=(DWORD number)
{
    _Number = number;

    return *this;
}

/// <summary>
/// Gets the message that describes this error.
/// </summary>
inline std::wstring error_t::Message() const noexcept
{
    std::wstring Message;

    Message.reserve(256);

    ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, _Number, 0, (LPWSTR) Message.c_str(), (DWORD) Message.capacity(), nullptr);

    return Message;
}


/** $VER: Error.h (2024.03.12) P. Stuer - Encapsulates the Win32 error number. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <windows.h>

#include <string>

class Error
{
public:
    Error() : _Number(NOERROR) { }

    Error(const Error &);
    Error & operator=(const Error &);
    Error(Error &&) = delete;
    Error & operator=(Error &&) = delete;

    virtual ~Error() { }

    Error(DWORD number) : _Number(number) { }
    Error & operator=(DWORD);

    operator DWORD() const noexcept { return _Number; }

    std::wstring Message() const noexcept;

private:
    DWORD _Number;
};

inline Error::Error(const Error & other)
{
    _Number= other._Number;
}

inline Error & Error::operator=(const Error & other)
{
    _Number = other._Number;

    return *this;
}

inline Error & Error::operator=(DWORD number)
{
    _Number = number;

    return *this;
}

/// <summary>
/// Gets the message that describes this error.
/// </summary>
inline std::wstring Error::Message() const noexcept
{
    std::wstring Message;

    Message.reserve(256);

    ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, _Number, 0, (LPWSTR) Message.c_str(), (DWORD) Message.capacity(), nullptr);

    return Message;
}

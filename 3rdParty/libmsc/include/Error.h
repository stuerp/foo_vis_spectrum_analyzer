
/** $VER: error_t.h (2025.09.30) P. Stuer - Encapsulates the Win32 error number. **/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <SDKDDKVer.h>
#include <windows.h>

#include <string>

namespace msc
{

class error_t
{
public:
    constexpr error_t() noexcept = default;

    explicit constexpr error_t(DWORD number) noexcept : _Number(number) { }

    constexpr error_t(const error_t &) noexcept = default;
    constexpr error_t(error_t &&) noexcept = default;

    constexpr error_t & operator=(const error_t &) noexcept = default;
    constexpr error_t & operator=(error_t &&) noexcept = default;

    ~error_t() = default;

    constexpr error_t & operator=(DWORD number) noexcept
    {
        _Number = number;

        return *this;
    }

    [[nodiscard]]
    explicit constexpr operator DWORD() const noexcept
    {
        return _Number;
    }

    [[nodiscard]]
    constexpr DWORD Number() const noexcept
    {
        return _Number;
    }

    [[nodiscard]]
    std::wstring Message() const;

private:
    struct local_free_t
    {
        void operator()(wchar_t * value) const noexcept
        {
            if (value != nullptr)
                ::LocalFree(value);
        }
    };

private:
    DWORD _Number { ERROR_SUCCESS };
};

inline std::wstring error_t::Message() const
{
    wchar_t * Text = nullptr;

    const DWORD Length = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, _Number, 0, (wchar_t *) &Text, 0, nullptr);

    if ((Length == 0) || (Text == nullptr))
        return { };

    const std::unique_ptr<wchar_t, local_free_t> Temp(Text);

    size_t n = Length;

    while (n > 0)
    {
        const wchar_t w = Text[n - 1];

        if (w != L'\r' && w != L'\n' && w != L' ' && w != L'\t')
            break;

        --n;
    }

    return std::wstring(Text, n);
}

}

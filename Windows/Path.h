
/** $VER: Path.h (2026.02.21) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <string>

#include <pathcch.h>

#pragma comment(lib, "pathcch")

class path_t
{
public:
    path_t() { }

    path_t(const path_t &);
    path_t & operator =(const path_t &);
    path_t(path_t &&);
    path_t & operator =(path_t &&);

    virtual ~path_t() { }

    path_t(const std::wstring & text) noexcept { _Text = text; }
    path_t & operator =(const std::wstring & text) noexcept { _Text = text; return *this; }

    operator const std::wstring & () const noexcept { return _Text; }
    operator LPCWSTR () const noexcept { return _Text.c_str(); }

    std::wstring c_str() const noexcept { return _Text; }

    static HRESULT AddExtension(const path_t & src, const std::wstring & extension, path_t & dst) noexcept;
    static HRESULT AddExtension(const std::wstring & src, const std::wstring & extension, path_t & dst) noexcept;

    static HRESULT Combine(const path_t & src, const path_t & part, path_t & dst) noexcept;
    static HRESULT Combine(const path_t & src, const WCHAR * part, path_t & dst) noexcept;

private:
    std::wstring _Text;
};

inline path_t::path_t(const path_t & other)
{
    _Text = other._Text;
}

inline path_t & path_t::operator =(const path_t & other)
{
    _Text = other._Text;

    return *this;
}

inline path_t::path_t(path_t && other)
{
    *this = ::std::move(other);
}

inline path_t & path_t::operator =(path_t && other)
{
    if (this != &other)
    {
        _Text = other._Text;
        other._Text = nullptr;
    }

    return *this;
}

inline HRESULT path_t::AddExtension(const path_t & src, const std::wstring & extension, path_t & dst) noexcept
{
    return AddExtension(src._Text, extension, dst);
}

inline HRESULT path_t::AddExtension(const std::wstring & src, const std::wstring & extension, path_t & dst) noexcept
{
    WCHAR Data[MAX_PATH];

    ::wcscpy_s(Data, _countof(Data), src.data());

    HRESULT hr = ::PathCchAddExtension(Data, _countof(Data), extension.c_str());

    if (SUCCEEDED(hr))
        dst._Text = Data;

    return hr;
}

inline HRESULT path_t::Combine(const path_t & src, const path_t & part, path_t & dst) noexcept
{
    return path_t::Combine(src, part._Text.data(), dst);
}

inline HRESULT path_t::Combine(const path_t & src, const WCHAR * part, path_t & dst) noexcept
{
    WCHAR Data[MAX_PATH];

    HRESULT hr = ::PathCchCombine(Data, _countof(Data), src._Text.data(), part);

    if (SUCCEEDED(hr))
        dst._Text = Data;

    return hr;
}

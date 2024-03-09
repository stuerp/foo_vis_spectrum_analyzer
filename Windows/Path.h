
/** $VER: Path.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <string>

#include <pathcch.h>

#pragma comment(lib, "pathcch")

class Path
{
public:
    Path() { }

    Path(const Path &);
    Path & operator =(const Path &);
    Path(Path &&);
    Path & operator =(Path &&);

    virtual ~Path() { }

    Path(const std::wstring & text) noexcept { _Text = text; }
    Path & operator =(const std::wstring & text) noexcept { _Text = text; return *this; }

    operator const std::wstring &() const noexcept { return _Text; }
    operator LPCWSTR() const noexcept { return _Text.c_str(); }

    static HRESULT AddExtension(const Path & src, const std::wstring & extension, Path & dst) noexcept;
    static HRESULT AddExtension(const std::wstring & src, const std::wstring & extension, Path & dst) noexcept;

    static HRESULT Combine(const Path & src, const Path & part, Path & dst) noexcept;
    static HRESULT Combine(const Path & src, const WCHAR * part, Path & dst) noexcept;

private:
    wstring _Text;
};

inline Path::Path(const Path & other)
{
    _Text = other._Text;
}

inline Path & Path::operator =(const Path & other)
{
    _Text = other._Text;

    return *this;
}

inline Path::Path(Path && other)
{
    *this = ::std::move(other);
}

inline Path & Path::operator =(Path && other)
{
    if (this != &other)
    {
        _Text = other._Text;
        other._Text = nullptr;
    }

    return *this;
}

inline HRESULT Path::AddExtension(const Path & src, const std::wstring & extension, Path & dst) noexcept
{
    return AddExtension(src._Text, extension, dst);
}

inline HRESULT Path::AddExtension(const std::wstring & src, const std::wstring & extension, Path & dst) noexcept
{
    WCHAR Data[MAX_PATH];

    ::wcscpy_s(Data, _countof(Data), src.data());

    HRESULT hr = ::PathCchAddExtension(Data, _countof(Data), extension.c_str());

    if (SUCCEEDED(hr))
        dst._Text = Data;

    return hr;
}

inline HRESULT Path::Combine(const Path & src, const Path & part, Path & dst) noexcept
{
    return Path::Combine(src, part._Text.data(), dst);
}

inline HRESULT Path::Combine(const Path & src, const WCHAR * part, Path & dst) noexcept
{
    WCHAR Data[MAX_PATH];

    HRESULT hr = ::PathCchCombine(Data, _countof(Data), src._Text.data(), part);

    if (SUCCEEDED(hr))
        dst._Text = Data;

    return hr;
}

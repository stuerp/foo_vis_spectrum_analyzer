
/** $VER: Convert.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <string>

#include <pfc/string_base.h>
#include <pfc/string-conv-lite.h>

class Convert
{
public:
    static const std::wstring From(const pfc::string & src) noexcept;
    static const pfc::string To(const std::wstring & src) noexcept;
};

inline const std::wstring Convert::From(const pfc::string & src) noexcept
{
    return std::wstring(pfc::wideFromUTF8(src).c_str());
}

inline const pfc::string Convert::To(const std::wstring & src) noexcept
{
    return pfc::string(pfc::utf8FromWide(src.c_str()));
}

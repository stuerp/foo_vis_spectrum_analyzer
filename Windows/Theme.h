
/** $VER: Theme.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

class theme_t
{
public:
    void Initialize(bool darkMode)
    {
        _DarkMode = darkMode;
    }

    COLORREF GetSysColor(int index) const noexcept;

private:
    bool _DarkMode;
};

extern theme_t _Theme;

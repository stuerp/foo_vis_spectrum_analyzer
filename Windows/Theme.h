
/** $VER: Theme.h (2024.01.21) P. Stuer **/

#pragma once

#include "framework.h"

class Theme
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

extern Theme _Theme;

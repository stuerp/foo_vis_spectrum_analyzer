
/** $VER: Theme.cpp (2024.03.09) P. Stuer **/

#include "Theme.h"

/// <summary>
/// Retrieves the current color of the specified display element, taking dark mode into account (Taken from libppui)
/// </summary>
COLORREF Theme::GetSysColor(int index) const noexcept
{
    if (!_DarkMode)
        return ::GetSysColor(index);

    switch (index)
    {
        case COLOR_MENU:
        case COLOR_BTNFACE:
        case COLOR_WINDOW:
        case COLOR_MENUBAR:
            return 0x202020;

        case COLOR_BTNSHADOW:
            return 0;

        case COLOR_WINDOWTEXT:
        case COLOR_MENUTEXT:
        case COLOR_BTNTEXT:
        case COLOR_CAPTIONTEXT:
            return 0xC0C0C0;

        case COLOR_BTNHIGHLIGHT:
        case COLOR_MENUHILIGHT:
            return 0x383838;

        case COLOR_HIGHLIGHT:
            return ::GetSysColor(COLOR_HOTLIGHT); //0x777777;

        case COLOR_HIGHLIGHTTEXT:
            return 0xFFFFFF; //0x101010;

        case COLOR_GRAYTEXT:
            return 0x777777;

        case COLOR_HOTLIGHT:
            return 0x0D69C56;

        default:
            return ::GetSysColor(index);
    }
}

Theme _Theme;

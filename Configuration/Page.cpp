
/** $VER: Page.cpp (2026.02.21) P. Stuer - Implements the configuration page base class. **/

#include "pch.h"

#include "Page.h"

/// <summary>
/// Initialize the dialog.
/// </summary>
BOOL page_t::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    auto dp = (const dialog_parameters_t *) lParam;

    _hParent = dp->_hWnd;
    _State = dp->_State;

    // Create the tooltip control.
    _ToolTipControl.Create(m_hWnd, nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE);

    if (_ToolTipControl.IsWindow())
    {
        _ToolTipControl.SetMaxTipWidth(200);
        ::SetWindowTheme(_ToolTipControl, _DarkMode ? L"DarkMode_Explorer" : nullptr, nullptr);
    }

    return TRUE;
}

/// <summary>
/// Sent to a window when the window is about to be hidden or shown.
/// </summary>
void page_t::OnShowWindow(BOOL isBeingShown, INT status) noexcept
{
    if (isBeingShown)
    {
        _IsInitializing = true;

        InitializeControls();

        _IsInitializing = false;
    }
}

/// <summary>
/// Returns a brush that the system uses to draw the dialog background. For layout debugging purposes.
/// </summary>
HBRUSH page_t::OnCtlColorDlg(HDC, HWND) const noexcept
{
#ifdef _DEBUG
    return ::CreateSolidBrush(RGB(248, 248, 248));
#else
    return FALSE;
#endif
}

/// <summary>
/// Sent when a window is being destroyed after the window is removed from the screen.
/// </summary>
void page_t::OnDestroy() noexcept
{
}

/// <summary>
/// Handles the UM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT page_t::OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
//  SetMsgHandled(TRUE);

    return 0;
}

/// <summary>
/// Validates and updates the notification from the up-down control.
/// </summary>
int page_t::ClampNewSpinPosition(LPNMUPDOWN nmud, int minValue, int maxValue) noexcept
{
    if ((nmud->iPos + nmud->iDelta) < minValue)
    {
        nmud->iPos   = minValue;
        nmud->iDelta = 0;
    }
    else
    if ((nmud->iPos + nmud->iDelta) > maxValue)
    {
        nmud->iPos   = maxValue;
        nmud->iDelta = 0;
    }

    return nmud->iPos + nmud->iDelta;
}

/// <summary>
/// Validates and updates the notification from the up-down control.
/// </summary>
double page_t::ClampNewSpinPosition(LPNMUPDOWN nmud, double minValue, double maxValue, double scaleFactor) noexcept
{
    if ((nmud->iPos + nmud->iDelta) / scaleFactor < minValue)
    {
        nmud->iPos   = (int) (minValue * scaleFactor);
        nmud->iDelta = 0;
    }
    else
    if ((nmud->iPos + nmud->iDelta) / scaleFactor > maxValue)
    {
        nmud->iPos   = (int) (maxValue * scaleFactor);
        nmud->iDelta = 0;
    }

    return (double) (nmud->iPos + nmud->iDelta) / scaleFactor;
}

/// <summary>
/// Sets the display value of an integer number.
/// </summary>
void page_t::SetInteger(int id, int64_t value) noexcept
{
    SetDlgItemTextW(id, pfc::wideFromUTF8(pfc::format_int(value)));
}

/// <summary>
/// Sets the display value of a real number.
/// </summary>
void page_t::SetDouble(int id, double value, unsigned width, unsigned precision) noexcept
{
    SetDlgItemTextW(id, pfc::wideFromUTF8(pfc::format_float(value, width, precision)));
}

/// <summary>
/// Sets the display value of a note number.
/// </summary>
void page_t::SetNote(int id, uint32_t noteNumber) noexcept
{
    static const WCHAR * Notes[] = { L"C%d", L"C#%d", L"D%d", L"D#%d", L"E%d", L"F%d", L"F#%d", L"G%d", L"G#%d", L"A%d", L"A#%d", L"B%d" };

    WCHAR Text[16] = { };

    const size_t NoteIndex = (size_t) (noteNumber % 12);
    const int Octave = (int) (noteNumber / 12);

    ::StringCchPrintfW(Text, _countof(Text), Notes[NoteIndex], Octave);

    SetDlgItemTextW(id, Text);
}

/// <summary>
/// Notifies the UI thread of the changed settings.
/// </summary>
void page_t::ConfigurationChanged(Settings settings) const noexcept
{
    if (_IsInitializing)
        return;

    ::PostMessageW(_hParent, UM_CONFIGURATION_CHANGED, (WPARAM) settings, 0);

    Log.AtDebug().Write(STR_COMPONENT_BASENAME " configuration dialog notified parent of configuration change (%08X).", (uint32_t) settings);
}

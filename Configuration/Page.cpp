
/** $VER: Page.cpp (2026.02.20) P. Stuer - Implements the configuration page base class. **/

#include "pch.h"

#include "Page.h"

/// <summary>
/// Terminates the controls of the dialog.
/// </summary>
/// <remarks>This is necessary to release the DirectX resources in case the control gets recreated later on.</remarks>
void page_t::Terminate() noexcept
{
    for (auto & Iter : _NumericEdits)
        Iter->Terminate();

    _NumericEdits.clear();
}

/// <summary>
/// Handles the UM_CONFIGURATION_CHANGED message.
/// </summary>
LRESULT page_t::OnConfigurationChanged(UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    SetMsgHandled(TRUE);

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

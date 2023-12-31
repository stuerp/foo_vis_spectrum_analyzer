
/** $VER: Log.cpp (2024.01.01) P. Stuer **/

#include "framework.h"

#include "Log.h"

#pragma hdrstop

namespace Log
{
static Level _Level = Level::Trace;

/// <summary>
/// Writes a message to the console.
/// </summary>
void Write(Level logLevel, const char * format, ...) noexcept
{
    if (logLevel < _Level)
        return;

    va_list va;

    va_start(va, format);

    console::printfv(format, va);

#ifdef _DEBUG
    CHAR Text[256];

    ::vsprintf_s(Text, _countof(Text), format, va);
    ::OutputDebugStringA(Text); ::OutputDebugStringA("\n");

#endif

    va_end(va);
}

}

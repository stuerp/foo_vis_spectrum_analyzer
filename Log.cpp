
/** $VER: Log.cpp (2024.03.09) P. Stuer **/

#include "Log.h"

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#define NOMINMAX
#include <helpers/foobar2000+atl.h>
#include <helpers/helpers.h>
#undef NOMINMAX

#include <strsafe.h>

#pragma hdrstop

namespace Log
{
#ifdef _DEBUG
static Level _Level = Level::Trace;
#else
static Level _Level = Level::Information;
#endif

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

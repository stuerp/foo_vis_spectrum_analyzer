
/** $VER: Support.h (2023.11.16) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Configuration.h"

inline void Log(LogLevel logLevel, const char * format, ...) noexcept
{
    if (logLevel < _Configuration._LogLevel)
        return;

    va_list va;

    va_start(va, format);

    console::printfv(format, va);

    va_end(va);
}

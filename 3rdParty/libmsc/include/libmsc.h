
/** $VER: libmsc.h (2025.09.22) P. Stuer - My Support Classes, The "Most Original Name" Winner **/

#pragma once

// UTF-8 Everywhere recommendation
#ifndef _UNICODE
#error Unicode character set compilation not enabled.
#endif

#include <SDKDDKVer.h>
#include <windows.h>

#include <filesystem>

namespace fs = std::filesystem;

#include "CriticalSection.h"
#include "Encoding.h"
#include "Enum.h"
#include "Exception.h"
#include "RAII.h"
#include "Stream.h"
#include "Support.h"

/// <summary>
/// A more sane way of representing a rectangle
/// </summary>
struct rect_t
{
    rect_t & operator = (const D2D1_RECT_F & other) noexcept
    {
        *this = other;

        return *this;
    }

    operator D2D1_RECT_F () const noexcept
    {
        return { x1, y1, x2, y2 };
    }

    D2D1_SIZE_F Size() const noexcept { return { std::abs(x1 - x2), std::abs(y1 - y2) }; }
    FLOAT Width() const noexcept { return std::abs(x2 - x1); }
    FLOAT Height() const noexcept { return std::abs(y2 - y1); }

    FLOAT x1;
    FLOAT y1;
    FLOAT x2;
    FLOAT y2;
};

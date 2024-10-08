
/** $VER: framework.h (2024.08.16) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#define NOMINMAX
#include <helpers/foobar2000+atl.h>
#include <helpers/helpers.h>
#undef NOMINMAX

#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <d2d1helper.h>
#include <dwrite.h>

#include <wincodec.h>

#include <stdlib.h>
#include <strsafe.h>

#include <algorithm>
#include <cmath>
#include <cassert>
#include <bit>

#include "CriticalSection.h"

#ifndef Assert
#if defined(DEBUG) || defined(_DEBUG)
#define Assert(b) do {if (!(b)) { ::OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif
#endif

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

#ifndef THIS_HINSTANCE
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define THIS_HINSTANCE ((HINSTANCE) &__ImageBase)
#endif

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

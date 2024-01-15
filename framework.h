
/** $VER: framework.h (2024.01.15) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX
#include <SDKDDKVer.h>
#undef NOMINMAX

#include <atlbase.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

#include <wincodec.h>

#include <stdlib.h>
#include <strsafe.h>

#include <cmath>

#include <helpers/foobar2000+atl.h>
#include <helpers/helpers.h>

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

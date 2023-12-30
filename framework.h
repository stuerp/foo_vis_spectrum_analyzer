
/** $VER: framework.h (2023.12.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

#include <SDKDDKVer.h>

#include <helpers/foobar2000+atl.h>
#include <helpers/helpers.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

#include <strsafe.h>
#include <math.h>

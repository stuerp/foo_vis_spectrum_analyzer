
/** $VER: CUIElement.cpp (2023.12.06) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "CUIElement.h"

#pragma hdrstop

#include "ui_extension.h"

namespace uie
{
static window_factory<CUIElement> _WindowFactory;
}

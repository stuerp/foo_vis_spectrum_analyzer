
/** $VER: Gradients.h (2024.03.16) P. Stuer - Built-in gradients. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <d2d1_2.h>
#include <vector>

#include "Constants.h"

typedef std::vector<D2D1_GRADIENT_STOP> GradientStops;

const GradientStops GetGradientStops(ColorScheme colorScheme);

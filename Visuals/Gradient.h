
/** $VER: Gradient.h (2026.10.02) P. Stuer - Built-in gradients. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <d2d1_2.h>
#include <vector>

#include "Constants.h"

class state_t;

struct gradient_stop_t : D2D1_GRADIENT_STOP
{
    GradientStopSource ColorSource; // The source of the color
    uint32_t ColorIndex;            // The index in the Windows or user interface color list

    void SetColor(const state_t * state) noexcept;
};

class gradient_t
{
public:
    static const std::vector<D2D1_GRADIENT_STOP> & GetBuiltIn(ColorScheme colorScheme) noexcept;

    static std::vector<gradient_stop_t> ConvertFormat(const std::vector<D2D1_GRADIENT_STOP> & gssIn);
    static std::vector<D2D1_GRADIENT_STOP> ConvertFormat(const std::vector<gradient_stop_t> & gssIn);

    static std::vector<D2D1_GRADIENT_STOP> CreateGradientStops(const std::vector<D2D1_COLOR_F> & colors);
};

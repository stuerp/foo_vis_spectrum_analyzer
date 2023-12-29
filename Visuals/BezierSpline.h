
/** $VER: BezierSpline.h (2023.12.29) P. Stuer - Bezier control point calculator. Based on https://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4211 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

class BezierSpline
{
public:
    static void GetControlPoints(const std::vector<D2D1_POINT_2F> knots, size_t count, std::vector<D2D1_POINT_2F> & firstControlPoints, std::vector<D2D1_POINT_2F> & secondControlPoints) noexcept;

private:
    static std::vector<FLOAT> GetFirstControlPoints(std::vector<FLOAT> rhs) noexcept;
};

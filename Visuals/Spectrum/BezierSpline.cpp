
/** $VER: BezierSpline.cpp (2023.12.30) P. Stuer - Based on https://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit **/

#include "pch.h"
#include "BezierSpline.h"

#pragma hdrstop

/// <summary>
/// Gets open-ended Bezier spline control points.
/// </summary>
void BezierSpline::GetControlPoints(const std::vector<D2D1_POINT_2F> knots, size_t count, std::vector<D2D1_POINT_2F> & firstControlPoints, std::vector<D2D1_POINT_2F> & secondControlPoints) noexcept
{
    if (count < 2)
        return;

    size_t n = count - 1;

    // Special case: Bezier curve should be a straight line.
    if (n == 1)
    {
        // 3P1 = 2P0 + P3
        firstControlPoints.push_back(D2D1::Point2F((2 * knots[0].x + knots[1].x) / 3, (2 * knots[0].y + knots[1].y) / 3));

        // P2 = 2P1 – P0
        secondControlPoints.push_back(D2D1::Point2F(2 * firstControlPoints[0].x - knots[0].x, 2 * firstControlPoints[0].y - knots[0].y));

        return;
    }

    // Right hand side vector
    std::vector<FLOAT> rhs(n);

    // Set right hand side X values
    for (size_t i = 1; i < n - 1; ++i)
        rhs[i] = 4.f * knots[i].x + 2.f * knots[i + 1].x;

    rhs[0] = knots[0].x + 2.f * knots[1].x;
    rhs[n - 1] = (8.f * knots[n - 1].x + knots[n].x) / 2.f;

    // Get first control points X-values
    std::vector<FLOAT> x = GetFirstControlPoints(rhs);

    // Set right hand side Y values
    for (size_t i = 1; i < n - 1; ++i)
        rhs[i] = 4.f * knots[i].y + 2.f * knots[i + 1].y;

    rhs[0] = knots[0].y + 2 * knots[1].y;
    rhs[n - 1] = (8.f * knots[n - 1].y + knots[n].y) / 2.f;

    // Get first control points Y-values
    std::vector<FLOAT> y = GetFirstControlPoints(rhs);

    // Fill output arrays.
    for (size_t i = 0; i < n; ++i)
    {
        // First control point
        firstControlPoints.push_back(D2D1::Point2F(x[i], y[i]));

        // Second control point
        if (i < n - 1)
            secondControlPoints.push_back(D2D1::Point2F(2.f * knots[i + 1].x - x[i + 1], 2.f * knots[i + 1].y - y[i + 1]));
        else
            secondControlPoints.push_back(D2D1::Point2F((knots[n].x + x[n - 1]) / 2.f, (knots[n].y + y[n - 1]) / 2.f));
    }
}

/// <summary>
/// Solves a tridiagonal system for one of coordinates (x or y) of first Bezier control points.
/// </summary>
std::vector<FLOAT> BezierSpline::GetFirstControlPoints(std::vector<FLOAT> rhs) noexcept
{
    size_t n = rhs.size();

    std::vector<FLOAT> x(n); // Solution vector.
    std::vector<FLOAT> t(n); // Temp workspace.

    FLOAT b = 2.f;

    x[0] = rhs[0] / b;

    // Decomposition and forward substitution.
    for (size_t i = 1; i < n; ++i)
    {
        t[i] = 1.f / b;
        b = (i < n - 1 ? 4.f : 3.5f) - t[i];
        x[i] = (rhs[i] - x[i - 1]) / b;
    }

    for (size_t i = 1; i < n; ++i)
        x[n - i - 1] -= t[n - i] * x[n - i]; // Backsubstitution.

    return x;
}

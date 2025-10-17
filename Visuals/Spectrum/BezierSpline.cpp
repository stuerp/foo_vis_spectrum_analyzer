
/** $VER: BezierSpline.cpp (2025.09.28) P. Stuer - Based on https://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit by Oleg V. Polikarpotchkin **/

#include "pch.h"
#include "BezierSpline.h"

#include <valarray>

#pragma hdrstop

/// <summary>
/// Gets open-ended Bezier spline control points.
/// </summary>
void bezier_spline_t::GetControlPoints(const std::vector<D2D1_POINT_2F> knots, std::vector<D2D1_POINT_2F> & firstControlPoints, std::vector<D2D1_POINT_2F> & secondControlPoints) noexcept
{
    if (knots.size() < 2)
        return;

    const size_t n = knots.size() - 1;

    // Special case: Bezier curve should be a straight line.
    if (n == 1)
    {
        // 3P1 = 2P0 + P3
        firstControlPoints.push_back(D2D1::Point2F
        (
            ((2.f * knots[0].x) + knots[1].x) / 3.f,
            ((2.f * knots[0].y) + knots[1].y) / 3.f
        ));

        // P2 = 2P1 – P0
        secondControlPoints.push_back(D2D1::Point2F
        (
            (2.f * firstControlPoints[0].x) - knots[0].x,
            (2.f * firstControlPoints[0].y) - knots[0].y
        ));

        return;
    }

    /* Calculate the first Bezier control points. */

    // Right hand side vector
    std::valarray<FLOAT> rhs(n);

    // Set right hand side X values
    {
        rhs[0] = knots[0].x + (2.f * knots[1].x);

        for (size_t i = 1; i < n - 1; ++i)
            rhs[i] = (4.f * knots[i].x) + (2.f * knots[i + 1].x);

        rhs[n - 1] = ((8.f * knots[n - 1].x) + knots[n].x) / 2.f;
    }

    auto x = GetFirstControlPoints(rhs);

    // Set right hand side Y values
    {
        rhs[0] = knots[0].y + (2 * knots[1].y);

        for (size_t i = 1; i < n - 1; ++i)
            rhs[i] = (4.f * knots[i].y) + (2.f * knots[i + 1].y);

        rhs[n - 1] = ((8.f * knots[n - 1].y) + knots[n].y) / 2.f;
    }

    auto y = GetFirstControlPoints(rhs);

    // Fill the output arrays.
    for (size_t i = 0; i < n - 1; ++i)
    {
        firstControlPoints.push_back(D2D1::Point2F(x[i], y[i]));
        secondControlPoints.push_back(D2D1::Point2F
        (
            (2.f * knots[i + 1].x) - x[i + 1],
            (2.f * knots[i + 1].y) - y[i + 1]
        ));
    }

    firstControlPoints.push_back(D2D1::Point2F(x[n - 1], y[n - 1]));
    secondControlPoints.push_back(D2D1::Point2F
    (
        (knots[n].x + x[n - 1]) / 2.f,
        (knots[n].y + y[n - 1]) / 2.f
    ));
}

/// <summary>
/// Solves a tridiagonal system for one of the coordinates (x or y) of first Bezier control points.
/// </summary>
std::valarray<FLOAT> bezier_spline_t::GetFirstControlPoints(std::valarray<FLOAT> rhs) noexcept
{
    size_t n = rhs.size();

    std::valarray<FLOAT> x(n); // Solution vector.
    std::valarray<FLOAT> t(n); // Temp workspace.

    FLOAT b = 2.f;

    x[0] = rhs[0] / b;

    // Decomposition and forward substitution.
    for (size_t i = 1; i < n; ++i)
    {
        t[i] = 1.f / b;
        b = (i < (n - 1) ? 4.f : 3.5f) - t[i];
        x[i] = (rhs[i] - x[i - 1]) / b;
    }

    for (size_t i = 1; i < n; ++i)
        x[n - i - 1] -= t[n - i] * x[n - i]; // Back substitution.

    return x;
}

/// <summary>
/// Bezier Spline methods
/// </summary>
/// <remarks>
/// Modified: Peter Lee (peterlee.com.cn@gmail.com)
///   Update: 2009-03-16
/// 
/// see also:
/// Draw a smooth curve through a set of 2D points with Bezier primitives
/// http://www.codeproject.com/KB/graphics/BezierSpline.aspx
/// By Oleg V. Polikarpotchkin
/// 
/// Algorithm Descripition:
/// 
/// To make a sequence of individual Bezier curves to be a spline, we
/// should calculate Bezier control points so that the spline curve
/// has two continuous derivatives at knot points.
/// 
/// Note: `[]` denotes subscript
///        `^` denotes supscript
///        `'` denotes first derivative
///       `''` denotes second derivative
///       
/// A Bezier curve on a single interval can be expressed as:
/// 
/// B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t)t^2 P2 + t^3 P3          (*)
/// 
/// where t is in [0,1], and
///     1. P0 - first knot point
///     2. P1 - first control point (close to P0)
///     3. P2 - second control point (close to P3)
///     4. P3 - second knot point
///     
/// The first derivative of (*) is:
/// 
/// B'(t) = -3(1-t)^2 P0 + 3(3t^2–4t+1) P1 + 3(2–3t)t P2 + 3t^2 P3
/// 
/// The second derivative of (*) is:
/// 
/// B''(t) = 6(1-t) P0 + 6(3t-2) P1 + 6(1–3t) P2 + 6t P3
/// 
/// Considering a set of piecewise Bezier curves with n+1 points
/// (Q[0..n]) and n subintervals, the (i-1)-th curve should connect
/// to the i-th one:
/// 
/// Q[0] = P0[1],
/// Q[1] = P0[2] = P3[1], ... , Q[i-1] = P0[i] = P3[i-1]  (i = 1..n)   (@)
/// 
/// At the i-th subinterval, the Bezier curve is:
/// 
/// B[i](t) = (1-t)^3 P0[i] + 3(1-t)^2 t P1[i] + 
///           3(1-t)t^2 P2[i] + t^3 P3[i]                 (i = 1..n)
/// 
/// applying (@):
/// 
/// B[i](t) = (1-t)^3 Q[i-1] + 3(1-t)^2 t P1[i] + 
///           3(1-t)t^2 P2[i] + t^3 Q[i]                  (i = 1..n)   (i)
///           
/// From (i), the first derivative at the i-th subinterval is:
/// 
/// B'[i](t) = -3(1-t)^2 Q[i-1] + 3(3t^2–4t+1) P1[i] +
///            3(2–3t)t P2[i] + 3t^2 Q[i]                 (i = 1..n)
/// 
/// Using the first derivative continuity condition:
/// 
/// B'[i-1](1) = B'[i](0)
/// 
/// we get:
/// 
/// P1[i] + P2[i-1] = 2Q[i-1]                             (i = 2..n)   (1)
/// 
/// From (i), the second derivative at the i-th subinterval is:
/// 
/// B''[i](t) = 6(1-t) Q[i-1] + 6(3t-2) P1[i] +
///             6(1-3t) P2[i] + 6t Q[i]                   (i = 1..n)
/// 
/// Using the second derivative continuity condition:
/// 
/// B''[i-1](1) = B''[i](0)
/// 
/// we get:
/// 
/// P1[i-1] + 2P1[i] = P2[i] + 2P2[i-1]                   (i = 2..n)   (2)
/// 
/// Then, using the so-called "natural conditions":
/// 
/// B''[1](0) = 0
/// 
/// B''[n](1) = 0
/// 
/// to the second derivative equations, and we get:
/// 
/// 2P1[1] - P2[1] = Q[0]                                              (3)
/// 
/// 2P2[n] - P1[n] = Q[n]                                              (4)
/// 
/// From (1)(2)(3)(4), we have 2n conditions for n first control points
/// P1[1..n], and n second control points P2[1..n].
/// 
/// Eliminating P2[1..n], we get (be patient to get :-) a set of n
/// equations for solving P1[1..n]:
/// 
///   2P1[1]   +  P1[2]   +            = Q[0] + 2Q[1]
///    P1[1]   + 4P1[2]   +    P1[3]   = 4Q[1] + 2Q[2]
///  ...
///    P1[i-1] + 4P1[i]   +    P1[i+1] = 4Q[i-1] + 2Q[i]
///  ...
///    P1[n-2] + 4P1[n-1] +    P1[n]   = 4Q[n-2] + 2Q[n-1]
///               P1[n-1] + 3.5P1[n]   = (8Q[n-1] + Q[n]) / 2
///  
/// From this set of equations, P1[1..n] are easy but tedious to solve.
/// </remarks>

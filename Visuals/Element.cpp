
/** $VER: Element.cpp (2024.04.08) P. Stuer - Base class for all visual elements **/

#include "pch.h"
#include "Element.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Sets the coordinate transform of the element.
/// </summary>
void Element::SetTransform(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) const noexcept
{
    D2D1::Matrix3x2F Transform = D2D1::Matrix3x2F::Identity();

    if (_GraphSettings->_FlipHorizontally)
        Transform = D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, bounds.right - bounds.left, 0.f);

    if (!_GraphSettings->_FlipVertically) // Negate because the GUI assumes the mathematical (bottom-left 0,0) coordinate system.
    {
        const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, bounds.bottom - bounds.top);

        Transform = Transform * FlipV;
    }

    D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(bounds.left, bounds.top);

    renderTarget->SetTransform(Transform * Translate);
}

/// <summary>
/// Resets the coordinate transform of the element.
/// </summary>
void Element::ResetTransform(ID2D1RenderTarget * renderTarget) const noexcept
{
    renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Returns true of the specified rectangle overlap vertically (while ignoring the horizontal position).
/// </summary>
bool Element::IsOverlappingHorizontally(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept
{
    return InRange(a.left, b.left, b.right) || InRange(a.right, b.left, b.right);
}

/// <summary>
/// Returns true of the specified rectangle overlap vertically (while ignoring the horizontal position).
/// </summary>
bool Element::IsOverlappingVertically(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept
{
    return InRange(a.top, b.top, b.bottom) || InRange(a.bottom, b.top, b.bottom);
}


/** $VER: Element.cpp (2024.04.08) P. Stuer - Base class for all visual elements **/

#include "pch.h"
#include "Element.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Sets the coordinate transform of the element.
/// </summary>
void element_t::SetTransform(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & rect) const noexcept
{
    D2D1::Matrix3x2F Transform = D2D1::Matrix3x2F::Identity();

    if (_GraphDescription->_FlipHorizontally)
        Transform = D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, rect.right - rect.left, 0.f);

    if (!_GraphDescription->_FlipVertically) // Negate because the GUI assumes the mathematical (bottom-left 0,0) coordinate system.
    {
        const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, rect.bottom - rect.top);

        Transform = Transform * FlipV;
    }

    const D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(rect.left, rect.top);

    deviceContext->SetTransform(Transform * Translate);
}

/// <summary>
/// Resets the coordinate transform of the element.
/// </summary>
void element_t::ResetTransform(ID2D1DeviceContext * deviceContext) const noexcept
{
    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Returns true of the specified rectangle overlap vertically (while ignoring the horizontal position).
/// </summary>
bool element_t::IsOverlappingHorizontally(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept
{
    return msc::InRange(a.left, b.left, b.right) || msc::InRange(a.right, b.left, b.right);
}

/// <summary>
/// Returns true of the specified rectangle overlap vertically (while ignoring the horizontal position).
/// </summary>
bool element_t::IsOverlappingVertically(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept
{
    return msc::InRange(a.top, b.top, b.bottom) || msc::InRange(a.bottom, b.top, b.bottom);
}

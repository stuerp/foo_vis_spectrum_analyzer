
/** $VER: Element.cpp (2024.03.29) P. Stuer - Base class for all visual elements **/

#include "Element.h"

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Sets the coordinate transform of the element.
/// </summary>
void Element::SetTransform(ID2D1RenderTarget * renderTarget) const noexcept
{
    D2D1::Matrix3x2F Transform = D2D1::Matrix3x2F::Identity();

    if (_GraphSettings->_FlipHorizontally)
        Transform = D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, _Size.width, 0.f);

    if (!_GraphSettings->_FlipVertically) // Negate because the GUI assumes the mathematical (bottom-left 0,0) coordinate system.
    {
        const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, _Size.height);

        Transform = Transform * FlipV;
    }

    D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_Bounds.left, _Bounds.top);

    renderTarget->SetTransform(Transform * Translate);
}

/// <summary>
/// Resets the coordinate transform of the element.
/// </summary>
void Element::ResetTransform(ID2D1RenderTarget * renderTarget) const noexcept
{
    renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

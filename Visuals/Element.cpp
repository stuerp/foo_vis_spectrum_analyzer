
/** $VER: Element.cpp (2026.11.12) P. Stuer - Base class for all visual elements **/

#include "pch.h"

#include "Element.h"

#pragma hdrstop

/// <summary>
/// Initializes some metrics of the element.
/// </summary>
void element_t::InitializeMetrics(const D2D1_RECT_F & rect) noexcept
{
    _Rect     = rect;
    _Size     = { std::abs(rect.right - rect.left), std::abs(rect.bottom - rect.top) };

    _Side     = std::min(_Size.width, _Size.height);
    _HalfSide = _Side / 2.f;

    _CenterX  = _Size.width  / 2.f;
    _CenterY  = _Size.height / 2.f;

    _State._SizeHasChanged = true;

    // The Direct2D device context was set to the correct DPI upon creation.
    _RenderContext.D2DDeviceContext->GetDpi(&_DPIX, &_DPIY);

    _PixelsPerDIP = _DPIX / USER_DEFAULT_SCREEN_DPI;
    _DIPsPerPixel = USER_DEFAULT_SCREEN_DPI / _DPIX;
}

/// <summary>
/// Sets the coordinate transform of the element.
/// </summary>
void element_t::SetTransform(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & rect) noexcept
{
    _Transform = D2D1::Matrix3x2F::Identity();

    if (_GraphOptions->_FlipHorizontally)
        _Transform = D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, rect.right - rect.left, 0.f);

    if (!_GraphOptions->_FlipVertically) // Negate because the GUI assumes the mathematical (bottom-left 0,0) coordinate system.
    {
        const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, rect.bottom - rect.top);

        _Transform = _Transform * FlipV;
    }

    const D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(rect.left, rect.top);

    _Transform = _Transform * Translate;

    deviceContext->SetTransform(_Transform);
}

/// <summary>
/// Resets the coordinate transform of the element.
/// </summary>
void element_t::ResetTransform(ID2D1DeviceContext * deviceContext) noexcept
{
    _Transform = D2D1::Matrix3x2F::Identity();

    deviceContext->SetTransform(_Transform);
}

/// <summary>
/// Returns true of the specified rectangles overlap vertically (while ignoring the horizontal position).
/// </summary>
bool element_t::IsOverlappingHorizontally(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept
{
    return msc::InRange(a.left, b.left, b.right) || msc::InRange(a.right, b.left, b.right);
}

/// <summary>
/// Returns true of the specified rectangles overlap vertically (while ignoring the horizontal position).
/// </summary>
bool element_t::IsOverlappingVertically(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept
{
    return msc::InRange(a.top, b.top, b.bottom) || msc::InRange(a.bottom, b.top, b.bottom);
}

/// <summary>
/// Calculates the horizontal offset to start rendering the visualization.
/// </summary>
FLOAT element_t::GetHOffset(HorizontalAlignment horizontalAlignment, FLOAT width) noexcept
{
    switch (horizontalAlignment)
    {
        case HorizontalAlignment::Center:
            return width / 2.f;

        default:
        case HorizontalAlignment::Fit:
        case HorizontalAlignment::Near:
            return 0.f;

        case HorizontalAlignment::Far:
            return width;
    }
}

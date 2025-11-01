
/** $VER: Element.h (2025.10.15) P. Stuer - Base class for all visual elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include "State.h"
#include "GraphDescription.h"
#include "Analysis.h"

#include "Direct2D.h"

#include "Style.h"

class element_t
{
public:
    element_t() : _State(), _GraphDescription(), _IsResized(true) {}

    virtual ~element_t() {}

    virtual void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept { }
    virtual void Move(const D2D1_RECT_F & rect) noexcept { }
    virtual void Render(ID2D1DeviceContext * deviceContext) noexcept { }
    virtual void Reset() noexcept { }
    virtual void Release() noexcept { }

    virtual const D2D1_RECT_F & GetRect() const noexcept { return _Rect; }
    virtual const D2D1_RECT_F & GetClientRect() const noexcept { return _Rect; };

    virtual void SetRect(const D2D1_RECT_F & rect) noexcept
    {
        _Rect = rect;

        // Derived metrics
        _Size = { std::abs(rect.right - rect.left), std::abs(rect.bottom - rect.top) };
        _ScaleFactor = std::min(_Size.width / 2.f, _Size.height  / 2.f); // For oscilloscope visualization.

        _IsResized = true;
    }

    virtual FLOAT GetWidth() const noexcept
    {
        return _Size.width;
    }

    virtual FLOAT GetHeight() const noexcept
    {
        return _Size.height;
    }

    virtual FLOAT GetLeft() const noexcept
    {
        return _Rect.left;
    }

    virtual FLOAT GetRight() const noexcept
    {
        return _Rect.right;
    }

    virtual void SetTransform(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & rect) const noexcept;
    virtual void ResetTransform(ID2D1DeviceContext * deviceContext) const noexcept;

    static bool IsOverlappingHorizontally(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept;
    static bool IsOverlappingVertically(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept;

    /// <summary>
    /// Calculates the horizontal offset to start rendering the visualization.
    /// </summary>
    static FLOAT GetHOffset(HorizontalAlignment horizontalAlignment, FLOAT width) noexcept
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

protected:
    void SafeRelease(style_t ** style) noexcept
    {
        if (*style != nullptr)
        {
            (*style)->DeleteDeviceSpecificResources();
            *style = nullptr;
        }
    }

protected:
    state_t * _State;
    const graph_description_t * _GraphDescription;
    const analysis_t * _Analysis;

    D2D1_RECT_F _Rect;
    D2D1_SIZE_F _Size;
    FLOAT _ScaleFactor;

    bool _IsResized;
};

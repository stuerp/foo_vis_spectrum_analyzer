
/** $VER: Element.h (2025.09.24) P. Stuer - Base class for all visual elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include "State.h"
#include "GraphSettings.h"
#include "Analysis.h"

#include "Direct2D.h"

#include "Style.h"

class element_t
{
public:
    element_t() : _State(), _GraphSettings(), _IsResized(true) {}

    virtual ~element_t() {}

    virtual void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept { };
    virtual void Move(const D2D1_RECT_F & rect) noexcept { };
    virtual void Render(ID2D1RenderTarget * renderTarget) noexcept { };
    virtual void Reset() noexcept { }

    virtual const D2D1_RECT_F & GetBounds() const noexcept { return _Bounds; }
    virtual const D2D1_RECT_F & GetClientBounds() const noexcept { return _Bounds; };

    virtual void SetBounds(const D2D1_RECT_F & bounds) noexcept
    {
        _Bounds = bounds;
        _Size = { std::abs(bounds.right - bounds.left), std::abs(bounds.bottom - bounds.top) };

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
        return _Bounds.left;
    }

    virtual FLOAT GetRight() const noexcept
    {
        return _Bounds.right;
    }

    virtual void ReleaseDeviceSpecificResources() noexcept { };

    virtual void SetTransform(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) const noexcept;
    virtual void ResetTransform(ID2D1RenderTarget * renderTarget) const noexcept;

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
            (*style)->ReleaseDeviceSpecificResources();
            *style = nullptr;
        }
    }

protected:
    state_t * _State;
    const graph_settings_t * _GraphSettings;
    const analysis_t * _Analysis;

    D2D1_RECT_F _Bounds;
    D2D1_SIZE_F _Size;

    bool _IsResized;
};

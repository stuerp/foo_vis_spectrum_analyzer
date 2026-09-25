
/** $VER: Element.h (2026.09.25) P. Stuer - Base class for all visual elements (both graph and visualizations). **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "State.h"
#include "Graphoptions.h"
#include "Analysis.h"

class element_t
{
public:
    element_t() = default;
    virtual ~element_t() = default;

    virtual void Move(const D2D1_RECT_F & rect) noexcept { }
    virtual void Render(ID2D1DeviceContext * deviceContext, IDXGISwapChain1 * swapChain) noexcept { }
    virtual void Reset() noexcept { }
    virtual void Release() noexcept { }

    virtual const D2D1_RECT_F & GetRect() const noexcept { return _Rect; }
    virtual const D2D1_RECT_F & GetClientRect() const noexcept { return _Rect; };

    virtual void SetTransform(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & rect) const noexcept;
    virtual void ResetTransform(ID2D1DeviceContext * deviceContext) const noexcept;

    virtual void OnConfigurationChange(ConfigurationChanges configurationChanges) noexcept { }

    static bool IsOverlappingHorizontally(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept;
    static bool IsOverlappingVertically(const D2D1_RECT_F & a, const D2D1_RECT_F & b) noexcept;

    static FLOAT GetHOffset(HorizontalAlignment horizontalAlignment, FLOAT width) noexcept;

protected:
    void InitializeMetrics(const D2D1_RECT_F & rect) noexcept;

protected:
    state_t * _State { nullptr };
    graph_options_t * _GraphOptions { nullptr };
    analysis_t * _Analysis { nullptr };

    bool _IsFirst { false };
    bool _IsLast { false };

    D2D1_RECT_F _Rect { };
    D2D1_SIZE_F _Size { };

    FLOAT _HalfSide { 0.f };
    FLOAT _Side { 0.f };

    bool _ForceElementToResize { true };
};

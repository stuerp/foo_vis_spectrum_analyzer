
/** $VER: Element.h (2024.04.06) P. Stuer - Base class for all visual elements. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include "State.h"
#include "GraphSettings.h"
#include "Analysis.h"

#include "Direct2D.h"

#include "Style.h"

class Element
{
public:
    Element() : _State(), _GraphSettings(), _IsResized(true) {}

    virtual ~Element() {}

    virtual const D2D1_RECT_F & GetBounds() const noexcept { return _Bounds; }

    virtual FLOAT GetLeft() const noexcept { return _Bounds.left; }
    virtual FLOAT GetRight() const noexcept { return _Bounds.right; }

    virtual void SetTransform(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) const noexcept;
    virtual void ResetTransform(ID2D1RenderTarget * renderTarget) const noexcept;

protected:
    void SafeRelease(Style ** style)
    {
        if (*style != nullptr)
        {
            (*style)->ReleaseDeviceSpecificResources();
            *style = nullptr;
        }
    }

protected:
    State * _State;
    const GraphSettings * _GraphSettings;
    const Analysis * _Analysis;

    D2D1_RECT_F _Bounds;
    D2D1_SIZE_F _Size;

    bool _IsResized;
};

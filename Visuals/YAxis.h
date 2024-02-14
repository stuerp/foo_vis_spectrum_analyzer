
/** $VER: YAxis.h (2024.02.07) P. Stuer - Implements the Y axis of a graph. **/

#pragma once

#include "framework.h"

#include "Element.h"
#include "Support.h"
#include "State.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the Y axis of a graph.
/// </summary>
class YAxis : public Element
{
public:
    YAxis() : _State(nullptr), _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _Bounds(), _Width(30.f), _Height() { }

    YAxis(const YAxis &) = delete;
    YAxis & operator=(const YAxis &) = delete;
    YAxis(YAxis &&) = delete;
    YAxis & operator=(YAxis &&) = delete;

    void Initialize(State * configuration);

    void Move(const D2D1_RECT_F & rect);

    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    FLOAT GetWidth() const { return _Width; }

private:
    State * _State;

    struct Label
    {
        double Amplitude;
        std::wstring Text;
        FLOAT y;
    };

    std::vector<Label> _Labels;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    // Device-independent resources
    D2D1_RECT_F _Bounds;
    FLOAT _Width;  // Width of the Y axis area (Font size-dependent).
    FLOAT _Height; // Height of a label

    CComPtr<IDWriteTextFormat> _TextFormat;
};

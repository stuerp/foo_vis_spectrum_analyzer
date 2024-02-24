
/** $VER: XAxis.h (2024.02.24) P. Stuer - Implements the X axis of a graph. **/

#pragma once

#include "framework.h"

#include "Element.h"
#include "Support.h"
#include "State.h"

#include "Analysis.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the X axis of a graph.
/// </summary>
class XAxis : public Element
{
public:
    XAxis() : _BandCount(), _LoFrequency(), _HiFrequency(), _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _Bounds(), _Height(30.f) { }

    XAxis(const XAxis &) = delete;
    XAxis & operator=(const XAxis &) = delete;
    XAxis(XAxis &&) = delete;
    XAxis & operator=(XAxis &&) = delete;

    void Initialize(State * state, const GraphSettings * settings, const FrequencyBands & frequencyBands) noexcept;

    void Move(const D2D1_RECT_F & rect);

    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    FLOAT GetHeight() const { return _Height; }

private:
    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

private:
    size_t _BandCount;
    double _LoFrequency;
    double _HiFrequency;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    struct Label
    {
        double Frequency;
        std::wstring Text;
        bool IsDimmed;

        D2D1_POINT_2F PointT;
        D2D1_POINT_2F PointB;

        D2D1_RECT_F RectT;
        D2D1_RECT_F RectB;
    };

    std::vector<Label> _Labels;

    D2D1_RECT_F _Bounds;
    FLOAT _Width;       // Width of a label
    FLOAT _Height;      // Height of the X axis area (Font size-dependent).

    // Device-independent resources
    CComPtr<IDWriteTextFormat> _TextFormat;

    Style * _LineStyle;
    Style * _TextStyle;
};

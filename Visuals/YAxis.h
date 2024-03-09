
/** $VER: YAxis.h (2024.03.09) P. Stuer - Implements the Y axis of a graph. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Element.h"
#include "State.h"
#include "FrequencyBand.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the Y axis of a graph.
/// </summary>
#pragma warning(disable: 4820)
class YAxis : public Element
{
public:
    YAxis() : _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _Bounds(), _Width(30.f), _Height() { }

    YAxis(const YAxis &) = delete;
    YAxis & operator=(const YAxis &) = delete;
    YAxis(YAxis &&) = delete;
    YAxis & operator=(YAxis &&) = delete;

    void Initialize(State * state, const GraphSettings * settings) noexcept;

    void Move(const D2D1_RECT_F & rect);

    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    FLOAT GetWidth() const { return _Width; }

private:
    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

private:
    bool _FlipVertically;

    struct Label
    {
        double Amplitude;
        std::wstring Text;

        D2D1_POINT_2F PointL;
        D2D1_POINT_2F PointR;

        D2D1_RECT_F RectL;
        D2D1_RECT_F RectR;
    };

    std::vector<Label> _Labels;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    // Device-independent resources
    D2D1_RECT_F _Bounds;
    FLOAT _Width;  // Width of the Y axis area (Font size-dependent).
    FLOAT _Height; // Height of a label

    CComPtr<IDWriteTextFormat> _TextFormat;

    Style * _LineStyle;
    Style * _TextStyle;

};

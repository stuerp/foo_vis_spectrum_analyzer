
/** $VER: XAxis.h (2023.12.30) P. Stuer - Implements the X axis of a graph. **/

#pragma once

#include "framework.h"
#include "Support.h"
#include "Configuration.h"

#include "FrequencyBand.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the X axis of a graph.
/// </summary>
class XAxis
{
public:
    XAxis() : _Configuration(), _Mode(), _LoFrequency(), _HiFrequency(), _NumBands(), _FontFamilyName(L"Segoe UI"), _FontSize(6.f), _Bounds(), _Height(30.f) { }

    XAxis(const XAxis &) = delete;
    XAxis & operator=(const XAxis &) = delete;
    XAxis(XAxis &&) = delete;
    XAxis & operator=(XAxis &&) = delete;

    void Initialize(const Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands);

    void Move(const D2D1_RECT_F & rect);

    void Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget);

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget);
    void ReleaseDeviceSpecificResources();

    FLOAT GetHeight() const { return _Height; }

private:
    const Configuration * _Configuration;

    XAxisMode _Mode;

    double _LoFrequency;
    double _HiFrequency;
    size_t _NumBands;

    D2D1_COLOR_F _TextColor;
    D2D1_COLOR_F _LineColor;
    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    struct Label
    {
        double Frequency;
        std::wstring Text;
        FLOAT x;
    };

    std::vector<Label> _Labels;

    D2D1_RECT_F _Bounds;
    FLOAT _Width;       // Width of a label
    FLOAT _Height;      // Height of the X axis area (Font size-dependent).

    // Device-independent resources
    CComPtr<IDWriteTextFormat> _TextFormat;

    // Device-specific resources
    CComPtr<ID2D1SolidColorBrush> _SolidBrush;
};

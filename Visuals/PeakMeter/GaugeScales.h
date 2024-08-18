
/** $VER: GaugeScales.h (2024.04.22) P. Stuer - Implements the gauge scales of the peak meter. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include <atlbase.h>

#include "Element.h"
#include "PeakMeterTypes.h"

class GaugeScales : public Element
{
public:
    GaugeScales() { };

    GaugeScales(const GaugeScales &) = delete;
    GaugeScales & operator=(const GaugeScales &) = delete;
    GaugeScales(GaugeScales &&) = delete;
    GaugeScales & operator=(GaugeScales &&) = delete;

    virtual ~GaugeScales() { }

    void Initialize(state_t * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

    FLOAT GetTextWidth() const noexcept
    {
        return _TextStyle->GetWidth();
    }

    FLOAT GetTextHeight() const noexcept
    {
        return _TextStyle->GetHeight();
    }

private:
    struct Label
    {
        std::wstring Text;
        double Amplitude;
        bool IsHidden;

        D2D1_POINT_2F Point1;
        D2D1_POINT_2F Point2;

        D2D1_RECT_F Rect1;
        D2D1_RECT_F Rect2;

        DWRITE_TEXT_ALIGNMENT _HAlignment;
        DWRITE_PARAGRAPH_ALIGNMENT _VAlignment;
    };

    std::vector<Label> _Labels;

    Style * _TextStyle;
    Style * _LineStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};


/** $VER: GaugeScales.h (2025.09.24) P. Stuer - Implements the gauge scales of the peak meter. **/

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

class gauge_scales_t : public element_t
{
public:
    gauge_scales_t() { };

    gauge_scales_t(const gauge_scales_t &) = delete;
    gauge_scales_t & operator=(const gauge_scales_t &) = delete;
    gauge_scales_t(gauge_scales_t &&) = delete;
    gauge_scales_t & operator=(gauge_scales_t &&) = delete;

    virtual ~gauge_scales_t() { }

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget) noexcept;
    void Reset() noexcept;
    void Resize() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

    FLOAT GetTextWidth() const noexcept
    {
        return _TextStyle->_Width;
    }

    FLOAT GetTextHeight() const noexcept
    {
        return _TextStyle->_Height;
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

    style_t * _TextStyle;
    style_t * _LineStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

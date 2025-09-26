
/** $VER: PeakReadOut.h (2025.09.24) P. Stuer - Implements the peak read out of the peak meter. **/

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

class peak_read_out_t : public element_t
{
public:
    peak_read_out_t() { };

    peak_read_out_t(const peak_read_out_t &) = delete;
    peak_read_out_t & operator=(const peak_read_out_t &) = delete;
    peak_read_out_t(peak_read_out_t &&) = delete;
    peak_read_out_t & operator=(peak_read_out_t &&) = delete;

    virtual ~peak_read_out_t() { }

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget) noexcept { };
    void Reset() noexcept;

    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) noexcept;

    bool IsVisible() const noexcept { return _TextStyle->IsEnabled(); }

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
    void RenderHorizontal(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) const noexcept;
    void RenderVertical(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) const noexcept;

private:
    style_t * _TextStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

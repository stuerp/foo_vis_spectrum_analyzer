
/** $VER: GaugeNames.h (2025.09.24) P. Stuer - Implements the gauge names of the peak meter. **/

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

class gauge_names_t : public element_t
{
public:
    gauge_names_t() { };

    gauge_names_t(const gauge_names_t &) = delete;
    gauge_names_t & operator=(const gauge_names_t &) = delete;
    gauge_names_t(gauge_names_t &&) = delete;
    gauge_names_t & operator=(gauge_names_t &&) = delete;

    virtual ~gauge_names_t() { }

    // element_t
    void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final { };
    void Reset() noexcept override final;

    void Resize() noexcept;
    void Render(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) noexcept;

    FLOAT GetTextWidth() const noexcept
    {
        return _TextStyle->_Width;
    }

    FLOAT GetTextHeight() const noexcept
    {
        return _TextStyle->_Height;
    }

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

private:
    void RenderHorizontal(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) const noexcept;
    void RenderVertical(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) const noexcept;

private:
    style_t * _TextStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    const FLOAT Offset = 4.f;
};

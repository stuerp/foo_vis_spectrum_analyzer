
/** $VER: PeakReadOut.h (2024.04.22) P. Stuer - Implements the peak read out of the peak meter. **/

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

class PeakReadOut : public Element
{
public:
    PeakReadOut() { };

    PeakReadOut(const PeakReadOut &) = delete;
    PeakReadOut & operator=(const PeakReadOut &) = delete;
    PeakReadOut(PeakReadOut &&) = delete;
    PeakReadOut & operator=(PeakReadOut &&) = delete;

    virtual ~PeakReadOut() { }

    void Initialize(state_t * state, const GraphSettings * settings, const analysis_t * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics);

    bool IsVisible() const noexcept { return _TextStyle->IsEnabled(); }

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
    void RenderHorizontal(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept;
    void RenderVertical(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept;

private:
    Style * _TextStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

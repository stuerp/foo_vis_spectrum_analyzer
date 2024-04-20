
/** $VER: PeakMeter.h (2024.04.20) P. Stuer - Represents a peak meter. **/

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

/// <summary>
/// Represents the metrics used to render the gauges.
/// </summary>
struct GaugeMetrics
{
    FLOAT _TotalBarGap;
    FLOAT _TickSize;
    FLOAT _TotalTickSize;

    FLOAT _BarHeight;
    FLOAT _BarWidth;
    FLOAT _TotalBarHeight;
    FLOAT _TotalBarWidth;
    FLOAT _Offset;

    double _dBFSZero;       // Relative position of 0 dBFS, 0.0 .. 1.0
};

class GaugeScales : public Element
{
public:
    GaugeScales() { };

    GaugeScales(const GaugeScales &) = delete;
    GaugeScales & operator=(const GaugeScales &) = delete;
    GaugeScales(GaugeScales &&) = delete;
    GaugeScales & operator=(GaugeScales &&) = delete;

    virtual ~GaugeScales() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
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
    };

    std::vector<Label> _Labels;

    Style * _TextStyle;
    Style * _LineStyle;
};

class GaugeNames : public Element
{
public:
    GaugeNames() { };

    GaugeNames(const GaugeNames &) = delete;
    GaugeNames & operator=(const GaugeNames &) = delete;
    GaugeNames(GaugeNames &&) = delete;
    GaugeNames & operator=(GaugeNames &&) = delete;

    virtual ~GaugeNames() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

    FLOAT GetTextWidth() const noexcept
    {
        return _TextStyle->GetHeight();
    }

    FLOAT GetTextHeight() const noexcept
    {
        return _TextStyle->GetHeight();
    }

private:
    void DrawHorizontalNames(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept;
    void DrawVerticalNames(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept;

private:
    Style * _TextStyle;
    Style * _RMSTextStyle;
};

class PeakMeter : public Element
{
public:
    PeakMeter();

    PeakMeter(const PeakMeter &) = delete;
    PeakMeter & operator=(const PeakMeter &) = delete;
    PeakMeter(PeakMeter &&) = delete;
    PeakMeter & operator=(PeakMeter &&) = delete;

    virtual ~PeakMeter() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

    void RenderGauges(ID2D1RenderTarget * renderTarget) const noexcept;

    void GetGaugeMetrics() noexcept;

//#define _DEBUG_RENDER

#ifdef _DEBUG_RENDER
    void DrawDebugRectangle(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & rect, const D2D1_COLOR_F & color) const noexcept
    {
        _DebugBrush->SetColor(color); renderTarget->DrawRectangle(rect, _DebugBrush);
    }
#endif

private:
    #pragma region Gauges

    GaugeMetrics _GaugeMetrics;

    D2D1_RECT_F _GBounds;   // Gauge bounds
    D2D1_SIZE_F _GSize;     // Gauge size

    #pragma endregion

    GaugeScales _GaugeScales;
    GaugeNames _GaugeNames;

    #pragma region Styling

    CComPtr<ID2D1Bitmap> _OpacityMask;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    Style * _BackgroundStyle;

    Style * _PeakStyle;
    Style * _Peak0dBStyle;
    Style * _MaxPeakStyle;

    Style * _RMSStyle;
    Style * _RMS0dBStyle;

    #pragma endregion
};

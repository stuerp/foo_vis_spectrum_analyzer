
/** $VER: PeakMeter.h (2024.04.21) P. Stuer - Represents a peak meter. **/

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

struct BOUNDS
{
    operator D2D1_RECT_F () const noexcept
    {
        return { x1, y1, x2, y2 };
    }

    D2D1_SIZE_F Size() const noexcept { return { std::abs(x1 - x2), std::abs(y1 - y2) }; }

    FLOAT x1;
    FLOAT y1;
    FLOAT x2;
    FLOAT y2;
};

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

        DWRITE_TEXT_ALIGNMENT _TextAlignment;
    };

    std::vector<Label> _Labels;

    Style * _TextStyle;
    Style * _LineStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
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

class RMSReadOut : public Element
{
public:
    RMSReadOut() { };

    RMSReadOut(const RMSReadOut &) = delete;
    RMSReadOut & operator=(const RMSReadOut &) = delete;
    RMSReadOut(RMSReadOut &&) = delete;
    RMSReadOut & operator=(RMSReadOut &&) = delete;

    virtual ~RMSReadOut() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
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

class Gauges : public Element
{
public:
    Gauges() { };

    Gauges(const Gauges &) = delete;
    Gauges & operator=(const Gauges &) = delete;
    Gauges(Gauges &&) = delete;
    Gauges & operator=(Gauges &&) = delete;

    virtual ~Gauges() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

    void GetGaugeMetrics(GaugeMetrics & gaugeMetrics) const noexcept;

private:
    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

private:
    CComPtr<ID2D1Bitmap> _OpacityMask;

    Style * _BackgroundStyle;

    Style * _PeakStyle;
    Style * _Peak0dBStyle;
    Style * _MaxPeakStyle;

    Style * _RMSStyle;
    Style * _RMS0dBStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
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
    GaugeMetrics _GaugeMetrics;

    Gauges      _Gauges;
    GaugeScales _GaugeScales;
    GaugeNames  _GaugeNames;
    RMSReadOut  _RMSReadOut;
};

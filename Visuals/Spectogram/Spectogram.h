
/** $VER: Spectogram.h (2024.04.28) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

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

#include <deque>

#include "SpectogramXAxis.h"
#include "SpectogramYAxis.h"

class Spectogram : public Element
{
public:
    Spectogram();

    Spectogram(const Spectogram &) = delete;
    Spectogram & operator=(const Spectogram &) = delete;
    Spectogram(Spectogram &&) = delete;
    Spectogram & operator=(Spectogram &&) = delete;

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget);
    void Reset();

    const D2D1_RECT_F & GetClientBounds() const noexcept { return _BitmapBounds; }

    void ReleaseDeviceSpecificResources();

private:
    bool Update() noexcept;

    void RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget) const noexcept;
    void RenderTimeAxis(ID2D1RenderTarget * renderTarget, bool top) const noexcept;
    void RenderYAxis(ID2D1RenderTarget * renderTarget, bool left) const noexcept;

    void InitYAxis() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);

    void Resize() noexcept;

private:
    D2D1_RECT_F _BitmapBounds;
    FLOAT _X;
    FLOAT _Y;
    double _PlaybackTime;
    double _TrackTime;
    bool _RequestErase;

    size_t _BandCount;
    double _LoFrequency;
    double _HiFrequency;

    struct TimeLabel
    {
        TimeLabel(const WCHAR * text, FLOAT x, FLOAT y = 0.f)
        {
            Text = text;
            X = x;
            Y = y;
        }

        std::wstring Text;
        FLOAT X;
        FLOAT Y;
    };

    std::deque<TimeLabel> _TimeLabels;

    struct FreqLabel
    {
        FreqLabel(const WCHAR * text, double frequency, bool isDimmed = false)
        {
            Text = text;
            Frequency = frequency;
            IsDimmed = isDimmed;
        }

        std::wstring Text;
        double Frequency;
        bool IsDimmed;
        bool IsHidden;

        D2D1_RECT_F Rect1;
        D2D1_RECT_F Rect2;
    };

    std::vector<FreqLabel> _FreqLabels;

    CComPtr<ID2D1BitmapRenderTarget> _BitmapRenderTarget;
    CComPtr<ID2D1Bitmap> _Bitmap;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    Style * _SpectogramStyle;

    Style * _XLineStyle;
    Style * _XTextStyle;

    Style * _YLineStyle;
    Style * _YTextStyle;

    Style * _NyquistMarkerStyle;

    D2D1_SIZE_F _BitmapSize;

    SpectogramXAxis _XAxis;
    SpectogramYAxis _YAxis;

    const FLOAT Offset = 4.f; // Distance between the tick and the text.
};

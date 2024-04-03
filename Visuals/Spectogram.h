
/** $VER: Spectogram.h (2024.04.02) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

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
#include "FrequencyBand.h"

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

    void Initialize(State * state, const GraphSettings * settings, const FrequencyBands & frequencyBands);
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate);
    void Reset();

    const D2D1_RECT_F & GetClientBounds() const noexcept { return _BitmapBounds; }

    void ReleaseDeviceSpecificResources();

private:
    void Update(const FrequencyBands & frequencyBands, double time, double sampleRate) noexcept;

    void RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) const noexcept;
    void RenderXAxis(ID2D1RenderTarget * renderTarget, bool top) const noexcept;
    void RenderYAxis(ID2D1RenderTarget * renderTarget, bool left) const noexcept;

    void InitYAxis(const FrequencyBands & frequencyBands) noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);

    void Resize() noexcept;

private:
    D2D1_RECT_F _BitmapBounds;
    FLOAT _X;
    double _PlaybackTime;
    double _TrackTime;
    bool _RequestErase;

    size_t _BandCount;
    double _LoFrequency;
    double _HiFrequency;

    struct XLabel
    {
        XLabel(const WCHAR * text, FLOAT x)
        {
            Text = text;
            X = x;
        }

        std::wstring Text;
        FLOAT X;
    };

    std::deque<XLabel> _XLabels;

    struct YLabel
    {
        YLabel(const WCHAR * text, double frequency, bool isDimmed = false)
        {
            Text = text;
            Frequency = frequency;
            IsDimmed = isDimmed;
        }

        std::wstring Text;
        double Frequency;
        bool IsDimmed;
    };

    std::vector<YLabel> _YLabels;

    CComPtr<ID2D1BitmapRenderTarget> _BitmapRenderTarget;
    CComPtr<ID2D1Bitmap> _Bitmap;

    Style * _SpectogramStyle;

    Style * _XLineStyle;
    Style * _XTextStyle;

    Style * _YLineStyle;
    Style * _YTextStyle;

    Style * _NyquistMarker;

    D2D1_SIZE_F _BitmapSize;

    SpectogramXAxis _XAxis;
    SpectogramYAxis _YAxis;

    const FLOAT Offset = 4.f; // Distance between the tick and the text.
};

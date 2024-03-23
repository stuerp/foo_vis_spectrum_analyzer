
/** $VER: Spectogram.h (2024.03.23) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

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

private:
    void Update(const FrequencyBands & frequencyBands, double time, double sampleRate) noexcept;

    void RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) const noexcept;
    void RenderXAxis(ID2D1RenderTarget * renderTarget, bool top) const noexcept;
    void RenderYAxis(ID2D1RenderTarget * renderTarget, bool left) const noexcept;

    void InitYAxis(const FrequencyBands & frequencyBands) noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

private:
    D2D1_RECT_F _Bounds;
    D2D1_SIZE_F _Size;

    FLOAT _X;
    double _PlaybackTime;
    double _TrackTime;
    bool _RequestErase;

    size_t _BandCount;
    double _LoFrequency;
    double _HiFrequency;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    FLOAT _XTextWidth;  // Width of a label
    FLOAT _XTextHeight; // Height of the X axis area (Font size-dependent).

    FLOAT _YTextWidth;  // Width of a label
    FLOAT _YTextHeight; // Height of the Y axis area (Font size-dependent).

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

    CComPtr<IDWriteTextFormat> _XTextFormat;
    CComPtr<IDWriteTextFormat> _YTextFormat;

    CComPtr<ID2D1BitmapRenderTarget> _BitmapRenderTarget;
    CComPtr<ID2D1Bitmap> _Bitmap;

    Style * _SpectogramStyle;

    Style * _XAxisLineStyle;
    Style * _XAxisTextStyle;

    Style * _YAxisLineStyle;
    Style * _YAxisTextStyle;

    Style * _NyquistMarker;

    D2D1_SIZE_F _BitmapSize;

    const FLOAT Offset = 4.f; // Distance between the tick and the text.
};

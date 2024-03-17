
/** $VER: Spectogram.h (2024.03.16) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

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

    void Initialize(State * state, const GraphSettings * settings);
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate, double time);
    void Reset();

private:
    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

private:
    D2D1_RECT_F _Bounds;
    D2D1_SIZE_F _Size;

    uint32_t _X;
    double _Time;
    bool _RequestErase;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    FLOAT _TextWidth;   // Width of a label
    FLOAT _TextHeight;  // Height of the X axis area (Font size-dependent).

    struct Label
    {
        Label(const WCHAR * text, FLOAT x)
        {
            _Text = text;
            _X = x;
        }

        std::wstring _Text;
        FLOAT _X;
    };

    std::deque<Label> _Labels;

    CComPtr<IDWriteTextFormat> _TextFormat;

    CComPtr<ID2D1BitmapRenderTarget> _BitmapRenderTarget;
    CComPtr<ID2D1Bitmap> _Bitmap;

    Style * _ForegroundStyle;

    Style * _XAxisLineStyle;
    Style * _XAxisTextStyle;
};

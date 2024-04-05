
/** $VER: PeakMeter.h (2024.04.05) P. Stuer - Represents a peak meter. **/

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
#include "Analysis.h"

class PeakMeter : public Element
{
public:
    PeakMeter();

    PeakMeter(const PeakMeter &) = delete;
    PeakMeter & operator=(const PeakMeter &) = delete;
    PeakMeter(PeakMeter &&) = delete;
    PeakMeter & operator=(PeakMeter &&) = delete;

    virtual ~PeakMeter() { }

    void Initialize(State * state, const GraphSettings * settings);
    void Move(const D2D1_RECT_F & rect);
    void Render(ID2D1RenderTarget * renderTarget, Analysis & analysis);
    void Reset();

    void ReleaseDeviceSpecificResources() noexcept;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;

    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

    void DrawScale(ID2D1RenderTarget * renderTarget) const noexcept;
    void DrawMeters(ID2D1RenderTarget * renderTarget, Analysis & analysis) const noexcept;

    void Resize() noexcept;

private:
    D2D1_RECT_F _ClientRect;
    D2D1_SIZE_F _ClientSize;

    struct Label
    {
        double Amplitude;
        std::wstring Text;

        D2D1_POINT_2F PointL;
        D2D1_POINT_2F PointR;

        D2D1_RECT_F RectL;
        D2D1_RECT_F RectR;
    };

    std::vector<Label> _Labels;

    FLOAT _XMin;
    FLOAT _XMax;

    FLOAT _YMin;
    FLOAT _YMax;

    CComPtr<ID2D1Bitmap> _OpacityMask;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    Style * _BackgroundStyle;

    Style * _PeakStyle;
    Style * _RMSStyle;

    Style * _XTextStyle;
    Style * _YTextStyle;
    Style * _YLineStyle;
};

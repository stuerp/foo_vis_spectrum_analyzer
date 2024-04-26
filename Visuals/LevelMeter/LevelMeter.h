
/** $VER: LevelMeter.h (2024.04.26) P. Stuer - Implements a left/right/mid/side level meter. **/

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

class LevelMeter : public Element
{
public:
    LevelMeter();

    LevelMeter(const LevelMeter &) = delete;
    LevelMeter & operator=(const LevelMeter &) = delete;
    LevelMeter(LevelMeter &&) = delete;
    LevelMeter & operator=(LevelMeter &&) = delete;

    virtual ~LevelMeter() { }

    void Initialize(State * state, const GraphSettings * settings, const Analysis * analysis);
    void Reset();
    void Move(const D2D1_RECT_F & rect);
    void Resize() noexcept;
    void Render(ID2D1RenderTarget * renderTarget);

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

private:
    CComPtr<ID2D1Bitmap> _OpacityMask;

    Style * _LeftRightStyle;
    Style * _LeftRightIndicatorStyle;
    Style * _MidSideStyle;
    Style * _MidSideIndicatorStyle;
    Style * _AxisStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

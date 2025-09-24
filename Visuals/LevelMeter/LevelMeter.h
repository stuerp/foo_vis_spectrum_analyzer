
/** $VER: LevelMeter.h (2025.09.24) P. Stuer - Implements a left/right/mid/side level meter. **/

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

class level_meter_t : public element_t
{
public:
    level_meter_t();

    level_meter_t(const level_meter_t &) = delete;
    level_meter_t & operator=(const level_meter_t &) = delete;
    level_meter_t(level_meter_t &&) = delete;
    level_meter_t & operator=(level_meter_t &&) = delete;

    virtual ~level_meter_t();

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget) noexcept;
    void Reset() noexcept;

    void Resize() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    HRESULT CreateOpacityMask(ID2D1RenderTarget * renderTarget) noexcept;

private:
    CComPtr<ID2D1Bitmap> _OpacityMask;

    style_t * _LeftRightStyle;
    style_t * _LeftRightIndicatorStyle;
    style_t * _MidSideStyle;
    style_t * _MidSideIndicatorStyle;
    style_t * _AxisStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

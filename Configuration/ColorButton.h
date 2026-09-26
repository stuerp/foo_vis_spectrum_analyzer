
/** $VER: ColorButton.h (2026.09.26) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <vector>

#include "DirectXControl.h"

/// <summary>
/// Implements a color control that can display gradients.
/// </summary>
class color_button_t : public CWindowImpl<color_button_t>, public directx_control_t
{
public:
    color_button_t() = default;

    color_button_t(const color_button_t &) = delete;
    color_button_t & operator=(const color_button_t &) = delete;
    color_button_t(color_button_t &&) = delete;
    color_button_t & operator=(color_button_t &&) = delete;

    virtual ~color_button_t() = default;

    void Initialize(HWND hWnd) noexcept;
    void Terminate() noexcept;

    void SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops) noexcept;

    void GetColor(D2D1_COLOR_F & color) const noexcept;
    void SetColor(const D2D1_COLOR_F & color) noexcept;
    void SetColor(COLORREF color) noexcept;

private:
    void OnPaint(HDC) noexcept;
    LRESULT OnLButtonDown(UINT, CPoint) noexcept;

    void SendChangedNotification() const noexcept;

    DECLARE_WND_CLASS_EX(NULL, 0, COLOR_3DFACE)

    BEGIN_MSG_MAP(color_button_t)
        MSG_WM_SIZE(OnSize)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
    END_MSG_MAP()

private:
    HRESULT CreateDeviceSpecificResources() noexcept override final;
    void DeleteDeviceSpecificResources() noexcept override final;

    HRESULT CreatePatternBrush(ID2D1RenderTarget * renderTarget) noexcept;

private:
    // Device-specific resources
    ComPtr<ID2D1SolidColorBrush> _BackgroundBrush;
    ComPtr<ID2D1Brush> _Brush;
    ComPtr<ID2D1BitmapBrush> _PatternBrush;

private:
    D2D1_COLOR_F _Color { };
    std::vector<D2D1_GRADIENT_STOP> _GradientStops;
};

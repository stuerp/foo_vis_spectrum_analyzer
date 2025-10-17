
/** $VER: Oscilloscope.h (2025.10.17) P. Stuer - Implements an oscilloscope. **/

#pragma once

#include <pch.h>

#include "Element.h"

class oscilloscope_t : public element_t
{
public:
    oscilloscope_t();

    oscilloscope_t(const oscilloscope_t &) = delete;
    oscilloscope_t & operator=(const oscilloscope_t &) = delete;
    oscilloscope_t(oscilloscope_t &&) = delete;
    oscilloscope_t & operator=(oscilloscope_t &&) = delete;

    virtual ~oscilloscope_t();

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1DeviceContext * deviceContext) noexcept;
    void Reset() noexcept;

    void Resize() noexcept;

    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

private:
    HRESULT CreateSignalGeometry(const D2D1_SIZE_F & size, CComPtr<ID2D1PathGeometry> & geometry) noexcept;
    HRESULT CreateAxesCommandList() noexcept;

private:
    CComPtr<ID2D1StrokeStyle> _SignalStrokeStyle;
    CComPtr<ID2D1StrokeStyle1> _AxisStrokeStyle;

    struct label_t
    {
        std::wstring Text;
        double Amplitude;

        bool IsMin;
        bool IsMax;
    };

    std::vector<label_t> _Labels;

    style_t * _SignalLineStyle;

    style_t * _XAxisTextStyle;
    style_t * _XAxisLineStyle;

    style_t * _YAxisTextStyle;
    style_t * _YAxisLineStyle;

    style_t * _HorizontalGridLineStyle;
    style_t * _VerticalGridLineStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    CComPtr<ID2D1DeviceContext> _DeviceContext; // Device context used to render the phospor blur

    CComPtr<ID2D1Bitmap1> _FrontBuffer;
    CComPtr<ID2D1Bitmap1> _BackBuffer;

    CComPtr<ID2D1Effect> _GaussBlurEffect;
    CComPtr<ID2D1Effect> _ColorMatrixEffect;

    CComPtr<ID2D1CommandList> _AxesCommandList;
};

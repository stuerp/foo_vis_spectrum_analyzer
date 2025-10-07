
/** $VER: Oscilloscope.h (2025.10.05) P. Stuer - Implements an oscilloscope. **/

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
    void Render(ID2D1RenderTarget * renderTarget) noexcept;
    void Reset() noexcept;

    void Resize() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    CComPtr<ID2D1StrokeStyle> _SignalStrokeStyle;

    struct label_t
    {
        std::wstring Text;
        double Amplitude;
    };

    std::vector<label_t> _Labels;

    style_t * _SignalLineStyle;
    style_t * _HorizontalGridLineStyle;
    style_t * _VerticalGridLineStyle;

    style_t * _LineStyle;
    style_t * _TextStyle;
};

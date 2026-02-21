
/** $VER: Spectrogram.h (2025.11.26) P. Stuer - Represents a spectrum analysis as a 2D heat map. **/

#pragma once

#include <pch.h>

#include "Element.h"

class spectrogram_t : public element_t
{
public:
    spectrogram_t();

    spectrogram_t(const spectrogram_t &) = delete;
    spectrogram_t & operator=(const spectrogram_t &) = delete;
    spectrogram_t(spectrogram_t &&) = delete;
    spectrogram_t & operator=(spectrogram_t &&) = delete;

    virtual ~spectrogram_t();

    // element_t
    void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    void OnResize() noexcept;

    HRESULT UpdateBackBuffer(ID2D1DeviceContext * deviceContext) noexcept;

private:
    CComPtr<ID2D1DeviceContext> _DeviceContext; // Device context used to render the back buffer
    CComPtr<ID2D1Bitmap1> _BackBuffer;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif

    style_t * _SpectrogramStyle;

    style_t * _TimeLineStyle;
    style_t * _TimeTextStyle;

    style_t * _FreqLineStyle;
    style_t * _FreqTextStyle;

    style_t * _NyquistMarkerStyle;

    std::vector<frequency_bands_t> _Spectra;
    size_t _SpectrumIndex;
    D2D1_RECT_F _BackBufferRect;
};


/** $VER: Oscilloscope.h (2026.09.25) P. Stuer - Implements an oscilloscope. **/

#pragma once

#include <pch.h>

#include "OscilloscopeBase.h"

#include <Analyzers/Downsampler.h>

class oscilloscope_t : public oscilloscope_base_t
{
public:
    oscilloscope_t() = default;

    oscilloscope_t(const oscilloscope_t &) = delete;
    oscilloscope_t & operator=(const oscilloscope_t &) = delete;
    oscilloscope_t(oscilloscope_t &&) = delete;
    oscilloscope_t & operator=(oscilloscope_t &&) = delete;

    virtual ~oscilloscope_t() noexcept;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext, IDXGISwapChain1 * swapChain) noexcept override final;
    void Reset() noexcept override final { }

    // visualization_t
    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;

private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    HRESULT CreateSignalGeometry(const audio_chunk_impl & chunk, const D2D1_SIZE_F & size, ComPtr<ID2D1PathGeometry> & geometry) noexcept;
    HRESULT CreateStaticContent(uint32_t axesCount) noexcept;

private:
    downsampler_t _Downsampler;

    struct label_t
    {
        std::wstring Text;
        double Amplitude { 0. };

        bool IsMin { false };
        bool IsMax { false };
    };

    std::vector<label_t> _Labels;

    style_t _XAxisTextStyle;
    style_t _YAxisTextStyle;

    double _ChunkDuration { 0. };

    ComPtr<ID2D1CommandList> _StaticContext;
    size_t _AxesCount { 0 };
};

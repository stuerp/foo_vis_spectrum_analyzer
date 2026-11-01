
/** $VER: D3DTester.cpp (2026.11.01) P. Stuer - Implements a minimal Direct3D visualization for testing purposes. **/

#pragma once

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Visualization.h"
#include "TestProcessor.h"

namespace Test
{

// Render() draws a D3D11 triangle into a DirectComposition surface, then uses the caller's Direct2D device context on that same DXGI surface.
class d3d_tester_t final : public visualization_t
{
public:
    d3d_tester_t(state_t & state, render_context_t & renderContext) : visualization_t(state, renderContext) { }

    d3d_tester_t(const d3d_tester_t &) = delete;
    d3d_tester_t(d3d_tester_t &&) = delete;

    d3d_tester_t & operator=(const d3d_tester_t &) = delete;
    d3d_tester_t & operator=(d3d_tester_t &&) = delete;

    ~d3d_tester_t() noexcept;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render3D() noexcept override final;
    void Reset() noexcept override final;

    // visualization_t
    void Configure(graph_options_t * options, audio_processor_t * analysis, bool isFirst, bool isLast) noexcept override final;
    virtual bool UsesDirect3D() const noexcept override final { return true; }

private:
    HRESULT CreatePipeline() noexcept;

private:
    processor_t * _Processor;

    struct vertex_t
    {
        float Position[2];
        float Color[4];
    };

private:
    ComPtr<ID3D11VertexShader> _VertexShader;
    ComPtr<ID3D11PixelShader> _PixelShader;
    ComPtr<ID3D11InputLayout> _InputLayout;
    ComPtr<ID3D11Buffer> _VertexBuffer;
};

}

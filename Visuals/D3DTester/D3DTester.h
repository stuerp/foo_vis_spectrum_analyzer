
/** $VER: D3DTester.cpp (2026.10.07) P. Stuer - Implements a minimal Direct3D visualization for testing purposes. **/

#pragma once

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Visualization.h"

// Render() draws a D3D11 triangle into a DirectComposition surface, then uses the caller's Direct2D device context on that same DXGI surface.
class d3d_tester_t final : public visualization_t
{
public:
    d3d_tester_t() noexcept = default;

    d3d_tester_t(const d3d_tester_t &) = delete;
    d3d_tester_t & operator=(const d3d_tester_t &) = delete;
    d3d_tester_t(d3d_tester_t &&) = delete;
    d3d_tester_t & operator=(d3d_tester_t &&) = delete;

    ~d3d_tester_t() noexcept;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render3D(IDXGISwapChain1 * swapChain) noexcept override final;
    void Reset() noexcept;

    // visualization_t
    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept override final;

private:
    struct vertex_t
    {
        float Position[2];
        float Color[4];
    };

    HRESULT CreatePipeline() noexcept;

private:
    ComPtr<ID3D11Device> _D3DDevice;
    ComPtr<ID3D11DeviceContext> _D3DDeviceContext;

    ComPtr<ID3D11VertexShader> _VertexShader;
    ComPtr<ID3D11PixelShader> _PixelShader;
    ComPtr<ID3D11InputLayout> _InputLayout;
    ComPtr<ID3D11Buffer> _VertexBuffer;
};

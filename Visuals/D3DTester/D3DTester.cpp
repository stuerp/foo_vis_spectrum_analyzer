
/** $VER: D3DTester.cpp (2026.10.07) P. Stuer - Implements a minimal Direct3D visualization for testing purposes. **/

#include "pch.h"

#include "D3DTester.h"

#include <Direct3D.h>

#include "Resources.h"

namespace
{
    constexpr char ShaderSource[] = R"(
struct VSInput
{
    float2 Position : POSITION;
    float4 Color    : COLOR;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.Position = float4(input.Position, 0.0f, 1.0f);
    output.Color    = input.Color;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.Color;
}
)";
}

/// <summary>
/// Destroys this instance.
/// </summary>
d3d_tester_t::~d3d_tester_t() noexcept
{
    Reset();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void d3d_tester_t::Move(const D2D1_RECT_F & rect) noexcept
{
    InitializeMetrics(rect);
}

/// <summary>
/// Initializes this instance.
/// </summary>
void d3d_tester_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    Reset();

    _D3DDevice        = d3dDevice;
    _D3DDeviceContext = d3dDeviceContext;

    HRESULT hr = CreatePipeline();

    if (FAILED(hr))
        Reset();
}

/// <summary>
/// Creates the 3D render pipeline.
/// </summary>
HRESULT d3d_tester_t::CreatePipeline() noexcept
{
    ComPtr<ID3DBlob> VertextShaderByteCode;

    // Create the vertex shader.
    {
        ComPtr<ID3DBlob> ErrorMessages;

        HRESULT hr = Direct3D::CompileShader(ShaderSource, "D3DTester", "VSMain", "vs_4_0_level_9_1", VertextShaderByteCode.GetAddressOf(), ErrorMessages.GetAddressOf());

        if (FAILED(hr))
        {
            Log.AtFatal().Write(STR_COMPONENT_BASENAME " failed to compile vertex shader: %s (0x%08X)", (char *) ErrorMessages->GetBufferPointer(), (int) hr);

            return hr;
        }

        hr = _D3DDevice->CreateVertexShader(VertextShaderByteCode->GetBufferPointer(), VertextShaderByteCode->GetBufferSize(), nullptr, _VertexShader.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    // Create the pixel shader.
    {
        ComPtr<ID3DBlob> PixelShaderByteCode;
        ComPtr<ID3DBlob> ErrorMessages;

        HRESULT hr = Direct3D::CompileShader(ShaderSource, "D3DTester", "PSMain", "ps_4_0_level_9_1", PixelShaderByteCode.GetAddressOf(), ErrorMessages.GetAddressOf());

        if (FAILED(hr))
        {
            Log.AtFatal().Write(STR_COMPONENT_BASENAME " failed to compile pixel shader: %s (0x%08X)", (char *) ErrorMessages->GetBufferPointer(), (int) hr);

            return hr;
        }

        hr = _D3DDevice->CreatePixelShader(PixelShaderByteCode->GetBufferPointer(), PixelShaderByteCode->GetBufferSize(), nullptr, _PixelShader.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    // Create the input layout.
    {
        const D3D11_INPUT_ELEMENT_DESC ied[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,                 D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        HRESULT hr = _D3DDevice->CreateInputLayout(ied, _countof(ied), VertextShaderByteCode->GetBufferPointer(), VertextShaderByteCode->GetBufferSize(), _InputLayout.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    // Create the vertex buffer.
    {
        const vertex_t Vertices[] =
        {
            { {  0.0f,   0.70f }, { 1.f, 0.f, 0.f, 1.0f } },
            { {  0.70f, -0.70f }, { 0.f, 1.f, 0.f, 1.0f } },
            { { -0.70f, -0.70f }, { 0.f, 0.f, 1.f, 1.0f } },
        };

        const D3D11_BUFFER_DESC bd =
        {
            .ByteWidth           = (UINT) sizeof(Vertices),
            .Usage               = D3D11_USAGE_IMMUTABLE,
            .BindFlags           = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags      = 0,
            .MiscFlags           = 0,
            .StructureByteStride = sizeof(vertex_t),
        };

        const D3D11_SUBRESOURCE_DATA sd =
        {
            .pSysMem          = Vertices,
            .SysMemPitch      = 0,
            .SysMemSlicePitch = 0,
        };

        return _D3DDevice->CreateBuffer(&bd, &sd, _VertexBuffer.GetAddressOf());
    }
}

/// <summary>
/// Renders the 3D content.
/// </summary>
void d3d_tester_t::Render3D(IDXGISwapChain1 * swapChain) noexcept
{
    if ((swapChain == nullptr) || (_D3DDevice == nullptr))
        return;

    // Create and bind a render target view for the swap-chain back buffer.
    {
        ComPtr<ID3D11Texture2D> BackBuffer;

        HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer.GetAddressOf()));

        if (FAILED(hr))
            return;

        ComPtr<ID3D11RenderTargetView> RenderTarget;

        hr = _D3DDevice->CreateRenderTargetView(BackBuffer.Get(), nullptr, RenderTarget.GetAddressOf());

        if (FAILED(hr))
            return;

        _D3DDeviceContext->OMSetRenderTargets(1, RenderTarget.GetAddressOf(), nullptr);
    }

    // Configure a viewport to restrict rasterization.
    {
        const D3D11_VIEWPORT Viewport =
        {
            0.0f,
            0.0f,
            _Size.width,
            _Size.height,
            0.0f,
            1.0f
        };

        _D3DDeviceContext->RSSetViewports(1, &Viewport);
    }

    // Configure the graphics pipeline and draw the content.
    {
        constexpr UINT Stride = sizeof(vertex_t);
        constexpr UINT Offset = 0;

        ID3D11Buffer * VertexBuffers[] =
        {
            _VertexBuffer.Get()
        };

        _D3DDeviceContext->IASetInputLayout(_InputLayout.Get());
        _D3DDeviceContext->IASetVertexBuffers(0, _countof(VertexBuffers), VertexBuffers, &Stride, &Offset);
        _D3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _D3DDeviceContext->VSSetShader(_VertexShader.Get(), nullptr, 0);
        _D3DDeviceContext->PSSetShader(_PixelShader.Get(), nullptr, 0);

        // Do not call ClearRenderTargetView(). Preserve the background previously drawn by Direct2D.

        _D3DDeviceContext->Draw(3, 0);
    }

    // Unbind the back buffer so that Direct2D can use it again.
    {
        ID3D11RenderTargetView * NullRenderTarget = nullptr;

        _D3DDeviceContext->OMSetRenderTargets(1, &NullRenderTarget, nullptr);
    }

//  _D3DDeviceContext->Flush();
}

/// <summary>
/// Resets this instance.
/// </summary>
void d3d_tester_t::Reset() noexcept
{
    _VertexBuffer.Reset();
    _InputLayout.Reset();
    _PixelShader.Reset();
    _VertexShader.Reset();

    _D3DDeviceContext.Reset();
    _D3DDevice.Reset();
}

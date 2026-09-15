
/** $VER: StereoMeter.cpp (2026.09.14) P. Stuer - Implements a stereo meter. **/

#include <pch.h>

#include "StereoMeter.h"
#include "StereoMeterResources.h"
#include "Resources.h"

#include <d3dcompiler.h>

#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
stereo_meter_t::stereo_meter_t()
{
    _Rect = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
stereo_meter_t::~stereo_meter_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void stereo_meter_t::Initialize(state_t * state, graph_options_t * graphOptions, const analysis_t * analysis, bool isFirst, bool isLast, CComPtr<ID3D11Device> d3dDevice, CComPtr<ID3D11DeviceContext> d3dDeviceContext) noexcept
{
    _State            = state;
    _GraphOptions     = graphOptions;
    _Analysis         = analysis;

    _D3DDevice        = d3dDevice;
    _D3DDeviceContext = d3dDeviceContext;

    CreateDeviceIndependentResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void stereo_meter_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void stereo_meter_t::Reset() noexcept
{
    if (!_ForceElementToResize || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _ForceElementToResize = true;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void stereo_meter_t::Release() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void stereo_meter_t::Resize() noexcept
{
    if (!_ForceElementToResize || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    CreateSizeDependentResources();

    _ForceElementToResize = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void stereo_meter_t::Render(ID2D1DeviceContext * deviceContext, CComPtr<IDXGISwapChain1> swapChain) noexcept
{
    _SwapChain = swapChain;

    {
        ComPtr<ID3D11Texture2D> BackBuffer;

        // Get the Direct3D backbuffer.
        if (FAILED(_SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer))))
            return;

        if (FAILED(_D3DDevice->CreateRenderTargetView(BackBuffer.Get(), nullptr, &_BackBufferRTV)))
            return;
    }

    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    constexpr double Energy = 0.018;

    const uint32_t SelectedChannels = _GraphOptions->_SelectedChannels;                                // Mask containing the channels selected by the user.
    const uint32_t PairedChannels   = analysis_t::ChannelPairs[(size_t) _GraphOptions->_ChannelPair];  // Mask containing the channels selected by the user as a channel pair.

    const auto Vertices = _AudioProcessor.Process(_Analysis->_Chunk, Energy, SelectedChannels, PairedChannels);
    const size_t Size = Vertices.size() * sizeof(vertex_t);

    if (Size != 0)
    {
        if (Size > _Capacity)
        {
            _Capacity = std::max(Size, std::max<size_t>(_Capacity * 2, 4096));

            const D3D11_BUFFER_DESC d
            {
                .ByteWidth      = (UINT)_Capacity,
                .Usage          = D3D11_USAGE_DYNAMIC,
                .BindFlags      = D3D11_BIND_VERTEX_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            };

            _VertexBuffer.Reset();

            if (FAILED(_D3DDevice->CreateBuffer(&d, nullptr, &_VertexBuffer)))
                return;
        }

        D3D11_MAPPED_SUBRESOURCE ms{};

        if (FAILED(_D3DDeviceContext->Map(_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
            return;

        std::memcpy(ms.pData, Vertices.data(), Size);

        _D3DDeviceContext->Unmap(_VertexBuffer.Get(), 0);
    }

    {
        D3D11_MAPPED_SUBRESOURCE ms{};

        if (FAILED(_D3DDeviceContext->Map(_ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
            return;

        auto * c = (Constants *) ms.pData;

        c->InverseViewport[0] = 1.f / (float) _Width;
        c->InverseViewport[1] = 1.f / (float) _Height;

        c->Decay              = 0.80f; // std::pow(.04f, std::clamp(deltaTime, 0.f, .1f));

        c->Correlation        = 0.f; // (float) std::clamp(correlation, -1., 1.);

        c->PeakLevels[0]      = 0.f; // (float) values.LevelL;
        c->PeakLevels[1]      = 0.f; // (float) values.LevelR;
        c->PeakHolds[0]       = 0.f; // (float) values.HoldL;
        c->PeakHolds[1]       = 0.f; // (float) values.HoldR;

        c->ClipFlags[0]       = 0.f; // values.ClipL ? 1.f : 0.f;
        c->ClipFlags[1]       = 0.f; // values.ClipR ? 1.f : 0.f;

        c->ThresholdData[0]   = 0.f; // (float) values.Threshold;
        c->ThresholdData[1]   = 0.f; // (float) values.Threshold;
        c->OverThreshold[0]   = 0.f; // values.IsOverThresholdL ? 1.f : 0.f;
        c->OverThreshold[1]   = 0.f; // values.IsOverThresholdR ? 1.f : 0.f;

        c->Padding[0]         = c->Padding[1] = 0.0f;

        static constexpr float ScopeWidthScale  =  0.84f;
        static constexpr float ScopeHeightScale =  0.84f;
        static constexpr float ScopeCenterX     = -0.07f;
        static constexpr float ScopeCenterY     =  0.05f;

        c->ScopeScale[0]      = ScopeWidthScale;
        c->ScopeScale[1]      = ScopeHeightScale;

        c->ScopeOffset[0]     = ScopeCenterX;
        c->ScopeOffset[1]     = ScopeCenterY;

        _D3DDeviceContext->Unmap(_ConstantBuffer.Get(), 0);
    }

    {
        ID3D11Buffer * cb = _ConstantBuffer.Get();

        _D3DDeviceContext->VSSetConstantBuffers(0, 1, &cb);
        _D3DDeviceContext->PSSetConstantBuffers(0, 1, &cb);
    }

    // Set the viewport.
    {
        const D3D11_VIEWPORT vp
        {
            .TopLeftX = 0.f,
            .TopLeftY = 0.f,
            .Width    = (float) _Width,
            .Height   = (float) _Height,
            .MinDepth = 0.f,
            .MaxDepth = 1.f
        };

        _D3DDeviceContext->RSSetViewports(1, &vp);
    }

    const UINT RTVIndex = 1 - _PrevRTVIndex;

    {
        ID3D11RenderTargetView * Target = _PersistenceRTV[RTVIndex].Get();

        _D3DDeviceContext->OMSetRenderTargets(1, &Target, nullptr);

        _D3DDeviceContext->OMSetBlendState(_BlendStateOpaque.Get(), nullptr, ~0u);

        _D3DDeviceContext->IASetInputLayout(nullptr);
        _D3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        _D3DDeviceContext->VSSetShader(_PresentVS.Get(), nullptr, 0);
        _D3DDeviceContext->PSSetShader(_FadePS.Get(), nullptr, 0);

        ID3D11ShaderResourceView * Source = _PersistenceSRV[_PrevRTVIndex].Get();

        _D3DDeviceContext->PSSetShaderResources(0, 1, &Source);

        _D3DDeviceContext->PSSetSamplers(0, 1, _SamplerState.GetAddressOf());

        _D3DDeviceContext->Draw(3, 0);
    }

    {
        ID3D11ShaderResourceView * NullSRV = nullptr;

        _D3DDeviceContext->PSSetShaderResources(0, 1, &NullSRV);

        if (Size != 0)
        {
            _D3DDeviceContext->OMSetBlendState(_BlendStateAdd.Get(), nullptr, ~0u);

            ID3D11Buffer * vb = _VertexBuffer.Get();

            const UINT Stride = sizeof(vertex_t);
            const UINT Offset = 0;

            _D3DDeviceContext->IASetInputLayout(_InputLayout.Get());
            _D3DDeviceContext->IASetVertexBuffers(0, 1, &vb, &Stride, &Offset);

            _D3DDeviceContext->VSSetShader(_ScopeVS.Get(), nullptr, 0);
            _D3DDeviceContext->PSSetShader(_ScopePS.Get(), nullptr, 0);

            _D3DDeviceContext->Draw((UINT) Vertices.size(), 0);
        }

        // Draw the result in the backbuffer render target view.
        {
            ID3D11RenderTargetView * Target = _BackBufferRTV.Get();

            _D3DDeviceContext->OMSetRenderTargets(1, &Target, nullptr);
            _D3DDeviceContext->OMSetBlendState(_BlendStateOpaque.Get(), nullptr, ~0u);

            _D3DDeviceContext->IASetInputLayout(nullptr);

            _D3DDeviceContext->VSSetShader(_PresentVS.Get(), nullptr, 0);
            _D3DDeviceContext->PSSetShader(_PresentPS.Get(), nullptr, 0);

            ID3D11ShaderResourceView * Source = _PersistenceSRV[RTVIndex].Get();

            _D3DDeviceContext->PSSetShaderResources(0, 1, &Source);

            _D3DDeviceContext->Draw(3, 0); // 3 vertices of the enlarged triangle.
        }

        _D3DDeviceContext->PSSetShaderResources(0, 1, &NullSRV);
    }

    _PrevRTVIndex = RTVIndex;
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT stereo_meter_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void stereo_meter_t::DeleteDeviceIndependentResources() noexcept
{
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT stereo_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if ((_Size.width == 0.f) || _Size.height == 0.f)
        return E_FAIL;

    HRESULT hr = S_OK;

    CreateSizeDependentResources();
    CreatePipeline();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void stereo_meter_t::DeleteDeviceSpecificResources() noexcept
{
    ReleaseSizeDependentResources();
}

/// <summary>
/// 
/// </summary>
bool stereo_meter_t::CreateSizeDependentResources() noexcept
{
    {
        ComPtr<ID3D11Texture2D> BackBuffer;

        // Get the Direct3D backbuffer.
        if (FAILED(_SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer))))
            return false;

        if (FAILED(_D3DDevice->CreateRenderTargetView(BackBuffer.Get(), nullptr, &_BackBufferRTV)))
            return false;
    }

    const D3D11_TEXTURE2D_DESC td
    {
        .Width      = _Width,
        .Height     = _Height,
        .MipLevels  = 1,
        .ArraySize  = 1,
        .Format     = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .SampleDesc = { .Count = 1 },
        .BindFlags  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    };

    for (size_t i = 0; i < 2; ++i)
    {
        if (FAILED(_D3DDevice->CreateTexture2D(&td, nullptr, &_PersistenceTex[i])))
            return false;

        if (FAILED(_D3DDevice->CreateRenderTargetView(_PersistenceTex[i].Get(), nullptr, &_PersistenceRTV[i])))
            return false;

        if (FAILED(_D3DDevice->CreateShaderResourceView(_PersistenceTex[i].Get(), nullptr, &_PersistenceSRV[i])))
            return false;
    }

    ClearPersistence();

    return true;
}

/// <summary>
/// 
/// </summary>
void stereo_meter_t::ReleaseSizeDependentResources() noexcept
{
    _BackBufferRTV.Reset();

    for (size_t i = 0; i < _countof(_PersistenceTex); ++i)
    {
        _PersistenceTex[i].Reset();
        _PersistenceRTV[i].Reset();
        _PersistenceSRV[i].Reset();
    }
}

/// <summary>
/// 
/// </summary>
bool stereo_meter_t::CreatePipeline()
{
    const HMODULE hModule = ::GetModuleHandleA(STR_COMPONENT_FILENAME);

    if (hModule == NULL)
        return false;

    {
        ComPtr<ID3DBlob> Shader;

        if (!CompileShaderResource(hModule, IDR_SHADER_SCOPE, "Scope.hlsl", "MainVS", "vs_5_0", &Shader))
            return false;

        if (FAILED(_D3DDevice->CreateVertexShader(Shader->GetBufferPointer(), Shader->GetBufferSize(), nullptr, &_ScopeVS)))
            return false;

        const D3D11_INPUT_ELEMENT_DESC ied[] =
        {
            { "CENTER", 0,DXGI_FORMAT_R32G32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "CORNER", 0,DXGI_FORMAT_R32G32_FLOAT,    0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",  0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "ENERGY", 0,DXGI_FORMAT_R32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        if (FAILED(_D3DDevice->CreateInputLayout(ied, _countof(ied), Shader->GetBufferPointer(), Shader->GetBufferSize(), &_InputLayout)))
            return false;
    }

    {
        ComPtr<ID3DBlob> Shader;

        if (!CompileShaderResource(hModule, IDR_SHADER_SCOPE, "Scope.hlsl", "MainPS", "ps_5_0", &Shader))
            return false;

        if (FAILED(_D3DDevice->CreatePixelShader(Shader->GetBufferPointer(), Shader->GetBufferSize(), nullptr, &_ScopePS)))
            return false;
    }

    {
        ComPtr<ID3DBlob> Shader;

        if (!CompileShaderResource(hModule, IDR_SHADER_FULLSCREEN, "Fullscreen.hlsl", "MainVS", "vs_5_0", &Shader))
            return false;

        if (FAILED(_D3DDevice->CreateVertexShader(Shader->GetBufferPointer(), Shader->GetBufferSize(), nullptr, &_PresentVS)))
            return false;
    }

    {
        ComPtr<ID3DBlob> Shader;

        if (!CompileShaderResource(hModule, IDR_SHADER_FULLSCREEN, "Fullscreen.hlsl", "FadePS", "ps_5_0", &Shader))
            return false;

        if (FAILED(_D3DDevice->CreatePixelShader(Shader->GetBufferPointer(), Shader->GetBufferSize(), nullptr, &_FadePS)))
            return false;
    }

    {
        ComPtr<ID3DBlob> Shader;

        if (!CompileShaderResource(hModule, IDR_SHADER_FULLSCREEN, "Fullscreen.hlsl", "PresentPS", "ps_5_0", &Shader))
            return false;

        if (FAILED(_D3DDevice->CreatePixelShader(Shader->GetBufferPointer(), Shader->GetBufferSize(), nullptr, &_PresentPS)))
            return false;
    }

    {
        const D3D11_BUFFER_DESC bd
        {
            .ByteWidth      = 80,
            .Usage          = D3D11_USAGE_DYNAMIC,
            .BindFlags      = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        if (FAILED(_D3DDevice->CreateBuffer(&bd, nullptr, &_ConstantBuffer)))
            return false;
    }

    {
        const D3D11_BLEND_DESC bd
        {
            .RenderTarget =
            {
                {
                    .BlendEnable          = TRUE,

                    .SrcBlend              = D3D11_BLEND_ONE,
                    .DestBlend             = D3D11_BLEND_ONE,
                    .BlendOp               = D3D11_BLEND_OP_ADD,

                    .SrcBlendAlpha         = D3D11_BLEND_ONE,
                    .DestBlendAlpha        = D3D11_BLEND_ONE,
                    .BlendOpAlpha          = D3D11_BLEND_OP_ADD,

                    .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
                },
            }
        };

        if (FAILED(_D3DDevice->CreateBlendState(&bd, &_BlendStateAdd)))
            return false;
    }

    {
        const D3D11_BLEND_DESC bd
        {
            .RenderTarget =
            {
                {
                    .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
                }
            }
        };

        if (FAILED(_D3DDevice->CreateBlendState(&bd, &_BlendStateOpaque)))
            return false;
    }

    {
        const D3D11_SAMPLER_DESC sd
        {
            .Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR,

            .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,

//          .MipLODBias
//          .MaxAnisotropy
//          .ComparisonFunc
//          .BorderColor
//          .MinLOD
//          .MaxLOD
        };

        return SUCCEEDED(_D3DDevice->CreateSamplerState(&sd, &_SamplerState));
    }
}

/// <summary>
/// 
/// </summary>
void stereo_meter_t::ClearPersistence()
{
    const float Zero[4] = {};

    for (auto & rtv : _PersistenceRTV)
    {
        if (rtv == nullptr)
            continue;

        _D3DDeviceContext->ClearRenderTargetView(rtv.Get(), Zero);
    }
}

/// <summary>
/// Compiles a shader from a Windows resource.
/// </summary>
bool stereo_meter_t::CompileShaderResource(HMODULE hModule, UINT resourceId, const char* fileName, const char * entryPoint, const char * target, ID3DBlob ** code) noexcept
{
    if (code == nullptr)
        return false;

    *code = nullptr;

    const HRSRC ResourceInfo = ::FindResourceW(hModule, MAKEINTRESOURCEW(resourceId), RT_RCDATA);

    if (ResourceInfo == nullptr)
    {
        const DWORD error = ::GetLastError();

        std::string message = "FindResourceW failed for shader resource " + std::to_string(resourceId) + ". Error: " + std::to_string(error);

        ::MessageBoxA(_hWnd, message.c_str(), "Shader resource error", MB_OK | MB_ICONERROR);

        return false;
    }

    const DWORD ResourceSize = ::SizeofResource(hModule, ResourceInfo);

    if (ResourceSize == 0)
    {
        ::MessageBoxA(_hWnd, "The embedded shader resource is empty.", "Shader resource error", MB_OK | MB_ICONERROR);

        return false;
    }

    const HGLOBAL ResourceData = ::LoadResource(hModule, ResourceInfo);

    if (ResourceData == nullptr)
    {
        const DWORD error = ::GetLastError();

        std::string message = "LoadResource failed for shader resource " + std::to_string(resourceId) + ". Error: " + std::to_string(error);

        ::MessageBoxA(_hWnd, message.c_str(), "Shader resource error", MB_OK | MB_ICONERROR);

        return false;
    }

    const void * SourceCode = ::LockResource(ResourceData);

    if (SourceCode == nullptr)
    {
        ::MessageBoxA(_hWnd, "LockResource failed for the embedded shader.", "Shader resource error", MB_OK | MB_ICONERROR);

        return false;
    }

    UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
    Flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    ComPtr<ID3DBlob> Errors;

    const HRESULT hr = ::D3DCompile(SourceCode, static_cast<SIZE_T>(ResourceSize), fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, Flags, 0, code, &Errors);

    if (FAILED(hr))
    {
        std::string Message;

        if (Errors != nullptr && Errors->GetBufferPointer() != nullptr && Errors->GetBufferSize() != 0)
            Message.assign((const char *) Errors->GetBufferPointer(), Errors->GetBufferSize());
        else
            Message = "Shader compilation failed. HRESULT: 0x" + std::format("0x{:08X}", (uint32_t) hr);

        ::MessageBoxA(_hWnd, Message.c_str(), fileName, MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

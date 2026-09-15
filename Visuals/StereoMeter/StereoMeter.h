
/** $VER: StereoMeter.h (2026.09.14) P. Stuer - Implements a stereo meter. **/

#pragma once

#include <pch.h>

#include <wrl/client.h>

#include "Visualization.h"
#include "DSP.h"

using Microsoft::WRL::ComPtr;

class stereo_meter_t : public visualization_t
{
public:
    stereo_meter_t();

    stereo_meter_t(const stereo_meter_t &) = delete;
    stereo_meter_t & operator=(const stereo_meter_t &) = delete;
    stereo_meter_t(stereo_meter_t &&) = delete;
    stereo_meter_t & operator=(stereo_meter_t &&) = delete;

    virtual ~stereo_meter_t();

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext, CComPtr<IDXGISwapChain1> swapChain) noexcept override final;
    void Reset() noexcept override final;
    void Release() noexcept override final;

    // visualization_t
    void Initialize(state_t * state, graph_options_t * graphOptions, const analysis_t * analysis, bool isFirst, bool isLast, CComPtr<ID3D11Device> d3dDevice, CComPtr<ID3D11DeviceContext> d3dDeviceContext) noexcept;
    void Resize() noexcept;

private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    bool CreateSizeDependentResources() noexcept;
    void ReleaseSizeDependentResources() noexcept;

    bool CreatePipeline();
    bool CompileShaderResource(HMODULE hModule, UINT resourceId, const char * sourceName, const char * entryPoint, const char * target, ID3DBlob ** code) noexcept;

    void ClearPersistence();

    HWND _hWnd = NULL;

    UINT _Width  = 1;
    UINT _Height = 1;

    size_t _Capacity = 0;
    UINT _PrevRTVIndex = 0;

    // Device-dependent resources.
    ComPtr<ID3D11Device> _D3DDevice;
    ComPtr<ID3D11DeviceContext> _D3DDeviceContext;

    ComPtr<IDXGISwapChain1> _SwapChain;

    ComPtr<ID3D11RenderTargetView> _BackBufferRTV;

    ComPtr<ID3D11Texture2D> _PersistenceTex[2];
    ComPtr<ID3D11RenderTargetView> _PersistenceRTV[2];
    ComPtr<ID3D11ShaderResourceView> _PersistenceSRV[2];

    ComPtr<ID3D11VertexShader> _ScopeVS;
    ComPtr<ID3D11PixelShader> _ScopePS;

    ComPtr<ID3D11VertexShader> _PresentVS;
    ComPtr<ID3D11PixelShader> _FadePS;
    ComPtr<ID3D11PixelShader> _PresentPS;

    ComPtr<ID3D11InputLayout> _InputLayout;
    ComPtr<ID3D11Buffer> _ConstantBuffer;
    ComPtr<ID3D11BlendState> _BlendStateAdd;
    ComPtr<ID3D11BlendState> _BlendStateOpaque;
    ComPtr<ID3D11SamplerState> _SamplerState;

    ComPtr<ID3D11Buffer> _VertexBuffer;

    audio_processor_t _AudioProcessor;
};

struct Constants
{
    float InverseViewport[2];
    float Decay;
    float Correlation;
    float PeakLevels[2];
    float PeakHolds[2];
    float ClipFlags[2];
    float ThresholdData[2];
    float OverThreshold[2];
    float Padding[2];
    float ScopeScale[2];
    float ScopeOffset[2];
};

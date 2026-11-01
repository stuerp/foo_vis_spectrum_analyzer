
/** $VER: RenderContext.h (2026.10.31) P. Stuer - Render context. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include <d3d11.h>
#include <d2d1_2.h>

struct render_context_t
{
public:
    void Reset() noexcept
    {
        _D2DDeviceContext = nullptr;

        _D3DDevice = nullptr;
        _D3DDeviceContext = nullptr;
    }

public:
    HRESULT CreateD3DRenderTarget(UINT width, UINT height, FLOAT dpiX, FLOAT dpiY) noexcept;

    void ReleaseD3DRenderTarget() noexcept;

    void Begin3D() noexcept;
    void End3D() noexcept;

    void Composite3D(const D2D1_RECT_F & destinationRectangle, FLOAT opacity = 1.f) noexcept;

    bool IsValid() const noexcept { return (_D3DDevice && _D3DDeviceContext && _D3DRenderTargetView); };

public:
    ID3D11Device * _D3DDevice = nullptr;
    ID3D11DeviceContext * _D3DDeviceContext = nullptr;

    ID2D1DeviceContext * _D2DDeviceContext = nullptr;

private:
    ComPtr<ID3D11Texture2D>        _D3DTexture;
    ComPtr<ID3D11RenderTargetView> _D3DRenderTargetView;
    ComPtr<ID2D1Bitmap1>           _D3DBitmap;

    D3D11_VIEWPORT _D3DViewport = {};
};

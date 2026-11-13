
/** $VER: RenderContext.cpp (2026.11.01) P. Stuer - Render context **/

#include "pch.h"

#include "RenderContext.h"

#include <cassert>

/// <summary>
/// Creates and initializes the Direct3D render target,
/// </summary>
HRESULT render_context_t::CreateD3DRenderTarget(UINT width, UINT height, FLOAT dpiX, FLOAT dpiY) noexcept
{
    ReleaseD3DRenderTarget();

    assert(D3DDevice); assert(D2DDeviceContext);

    const D3D11_TEXTURE2D_DESC td =
    {
        .Width      = width,
        .Height     = height,
        .MipLevels  = 1,
        .ArraySize  = 1,
        .Format     = DXGI_FORMAT_B8G8R8A8_UNORM,
        .SampleDesc = { 1, 0 },
        .Usage      = D3D11_USAGE_DEFAULT,
        .BindFlags  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    };

    HRESULT hr = D3DDevice->CreateTexture2D(&td, nullptr, _D3DTexture.GetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = D3DDevice->CreateRenderTargetView(_D3DTexture.Get(), nullptr, _D3DRenderTargetView.GetAddressOf());

    if (FAILED(hr))
    {
        ReleaseD3DRenderTarget();

        return hr;
    }

    ComPtr<IDXGISurface> Surface;

    hr = _D3DTexture->QueryInterface(IID_PPV_ARGS(Surface.GetAddressOf()));

    if (FAILED(hr))
    {
        ReleaseD3DRenderTarget();

        return hr;
    }

    const D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);

    hr = D2DDeviceContext->CreateBitmapFromDxgiSurface(Surface.Get(), &bp, _D3DBitmap.GetAddressOf());

    if (FAILED(hr))
    {
        ReleaseD3DRenderTarget();

        return hr;
    }

    _D3DViewport.TopLeftX = 0.f;
    _D3DViewport.TopLeftY = 0.f;
    _D3DViewport.Width    = (FLOAT) width;
    _D3DViewport.Height   = (FLOAT) height;
    _D3DViewport.MinDepth = 0.f;
    _D3DViewport.MaxDepth = 1.f;

    return S_OK;
}

/// <summary>
/// Releases the Direct3D render target resources.
/// </summary>
void render_context_t::ReleaseD3DRenderTarget() noexcept
{
    _D3DBitmap.Reset();
    _D3DRenderTargetView.Reset();
    _D3DTexture.Reset();

    _D3DViewport = {};
}

/// <summary>
/// Binds and clears the offscreen D3D target.
/// </summary>
void render_context_t::Begin3D() noexcept
{
    assert(D3DDeviceContext); assert(_D3DRenderTargetView);

    constexpr FLOAT ClearColor[4] = { 0.f, 0.f, 0.f, 0.f };

    ID3D11RenderTargetView * RenderTargets[1] = { _D3DRenderTargetView.Get() };

    D3DDeviceContext->OMSetRenderTargets(_countof(RenderTargets), RenderTargets, nullptr);

    D3DDeviceContext->RSSetViewports(1, &_D3DViewport);

    D3DDeviceContext->ClearRenderTargetView(_D3DRenderTargetView.Get(), ClearColor);
}

/// <summary>
/// Unbinds the offscreen render target view before Direct2D samples the Direct3D texture.
/// </summary>
void render_context_t::End3D() noexcept
{
    assert(D3DDeviceContext);

    ID3D11RenderTargetView * RenderTargets[1] = { nullptr };

    D3DDeviceContext->OMSetRenderTargets(_countof(RenderTargets), RenderTargets, nullptr);
}

/// <summary>
/// Composite the Direct3D texture into the current Direct2D render pass using an ID2D1Bitmap1 wrapper.
/// </summary>
void render_context_t::Composite3D(const D2D1_RECT_F & destinationRectangle, FLOAT opacity) noexcept
{
    assert(D2DDeviceContext); assert(_D3DBitmap);

    D2DDeviceContext->DrawBitmap(_D3DBitmap.Get(), destinationRectangle, opacity, D2D1_INTERPOLATION_MODE_LINEAR, nullptr, nullptr);
}


/** $VER: RenderContext.h (2026.10.11) P. Stuer - Render context. **/

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
        D3DDevice = nullptr;
        D3DDeviceContext = nullptr;

        D2DDeviceContext = nullptr;

        SwapChain = nullptr;
    }

public:
    ID3D11Device * D3DDevice { };
    ID3D11DeviceContext * D3DDeviceContext { };

    ID2D1DeviceContext * D2DDeviceContext { };

    IDXGISwapChain1 * SwapChain { };
};

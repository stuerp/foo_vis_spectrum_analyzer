
/** $VER: CDirectXControl.h (2026.09.26) P. Stuer - Implements a base class for DirectX rendered controls. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include <d2d1.h>
#include <d2d1helper.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class directx_control_t
{
public:
    directx_control_t() = default;

    directx_control_t(const directx_control_t &) = delete;
    directx_control_t & operator=(const directx_control_t &) = delete;
    directx_control_t(directx_control_t &&) = delete;
    directx_control_t & operator=(directx_control_t &&) = delete;

    virtual ~directx_control_t() noexcept
    {
        DeleteDeviceIndependentResources();
    }

protected:
    virtual void OnSize(UINT type, CSize size) noexcept
    {
        if (_RenderTarget == nullptr)
            return;

        const auto Size = D2D1::SizeU((UINT32) size.cx, (UINT32) size.cy);

        _RenderTarget->Resize(Size);
    }

    #pragma region DirectX

    virtual HRESULT CreateDeviceIndependentResources() noexcept;
    virtual void DeleteDeviceIndependentResources() noexcept;

    virtual HRESULT CreateDeviceSpecificResources() noexcept;
    virtual void DeleteDeviceSpecificResources() noexcept;

    #pragma endregion

protected:
    HWND _hWnd { };
    bool _IsSubclassed { false };

    // Device-independent resources
    static constexpr bool _UseHardwareRendering = true;
    static constexpr bool _UseAntialiasing = true;

    // Device-specific resources
    ComPtr<ID2D1HwndRenderTarget> _RenderTarget;
};

#define NM_CHANGED (NM_RETURN)

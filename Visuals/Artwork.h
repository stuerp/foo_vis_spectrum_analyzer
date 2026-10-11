
/** $VER: Artwork.h (2026.10.11) P. Stuer  **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>
#include <d2d1.h>

#include "State.h"
#include "RenderContext.h"

#include <Constants.h>

class artwork_t
{
public:
    artwork_t() = default;

    virtual ~artwork_t()
    {
        DeleteDeviceSpecificResources();

        DeleteWICResources();
    }

    bool Empty() noexcept
    {
        msc::lock_t Lock(_CriticalSection);

        return (_FormatConverter == nullptr);
    }

    D2D1_SIZE_F Size() noexcept
    {
        msc::lock_t Lock(_CriticalSection);

        if (_Bitmap == nullptr)
            return { };

        return _Bitmap->GetSize();
    }

    void Render(render_context_t & renderContext, const D2D1_RECT_F & rect, const state_t * state) noexcept;

    HRESULT GetColors(uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold, std::vector<D2D1_COLOR_F> & colors) noexcept;

    HRESULT CreateWICResources(const uint8_t * data, size_t size) noexcept;
    HRESULT CreateWICResources(const std::wstring & filePath) noexcept;
    HRESULT DeleteWICResources() noexcept;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    void AdjustRect(const FitMode fitMode, const bool allowUpscaling, FLOAT & scalar, D2D1_RECT_F & rect) const noexcept;

private:
    msc::critical_section_t _CriticalSection;

    std::vector<uint8_t> _Raster;
    std::wstring _FilePath;

    ComPtr<IWICBitmapFrameDecode> _Frame;
    ComPtr<IWICFormatConverter> _FormatConverter;

    ComPtr<ID2D1Bitmap> _Bitmap;

    ComPtr<ID2D1Effect> _ScaleEffect;
    ComPtr<ID2D1Effect> _BlurEffect;
    ComPtr<ID2D1Effect> _OpacityEffect;
};

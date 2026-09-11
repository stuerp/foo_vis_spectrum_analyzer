
/** $VER: Artwork.h (2026.06.10) P. Stuer  **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>
#include <d2d1.h>

#include <RAII.h>

#include "State.h"
#include <Constants.h>

class artwork_t
{
public:
    artwork_t()
    {
//      SetStatus(Idle);
    }

    virtual ~artwork_t()
    {
        DeleteDeviceSpecificResources();

        DeleteWICResources();
    }

    D2D1_SIZE_F Size() const noexcept { return (_Bitmap != nullptr) ? _Bitmap->GetSize() : D2D1::SizeF(); }

    void Render(ID2D1DeviceContext * deviceContext, const D2D1_RECT_F & rect, const state_t * state) noexcept;

    ID2D1Bitmap * Bitmap() const noexcept { return _Bitmap; }

    HRESULT GetColors(std::vector<D2D1_COLOR_F> & colors, uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold) noexcept;

    HRESULT CreateWICResources(const uint8_t * data, size_t size) noexcept;
    HRESULT CreateWICResources(const std::wstring & filePath) noexcept;
    HRESULT DeleteWICResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;

private:
    void DeleteDeviceSpecificResources() noexcept;

private:
    void AdjustRect(_In_ const FitMode fitMode, _Out_ FLOAT & scalar, _Inout_ D2D1_RECT_F & rect) const noexcept;

private:
/*
    enum Status
    {
        Idle = 0,

        Initialized,    // A new artwork source has been set.

        GotBitmap,      // A new bitmap has been generated or the configuration parameters have changed.
        GotColors,      // Got the colors from the bitmap source.
    };

    void SetStatus(Status status) noexcept
    {
        msc::lock_t Lock(_CriticalSection);

        _Status = status;
    }
*/
private:
    msc::critical_section_t _CriticalSection;

    std::vector<uint8_t> _Raster;
    std::wstring _FilePath;

    CComPtr<IWICBitmapFrameDecode> _Frame;
    CComPtr<IWICFormatConverter> _FormatConverter;
    CComPtr<ID2D1Bitmap> _Bitmap;

    CComPtr<ID2D1Effect> _ScaleEffect;
    CComPtr<ID2D1Effect> _BlurEffect;
    CComPtr<ID2D1Effect> _OpacityEffect;

//  Status _Status;
};

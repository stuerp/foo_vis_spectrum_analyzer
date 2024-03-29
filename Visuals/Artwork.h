
/** $VER: Artwork.h (2024.03.11) P. Stuer  **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>
#include <d2d1_2.h>

#include "State.h"
#include "CriticalSection.h"

class Artwork
{
public:
    Artwork()
    {
        SetIdle();
    }

    virtual ~Artwork()
    {
        Release();
    }

    HRESULT Initialize(const uint8_t * data, size_t size) noexcept;
    HRESULT Initialize(const std::wstring & filePath) noexcept;

    HRESULT Realize(ID2D1RenderTarget * renderTarget) noexcept;
    HRESULT GetColors(std::vector<D2D1_COLOR_F> & colors, uint32_t colorCount, FLOAT lightnessThreshold, FLOAT transparencyThreshold) noexcept;

    D2D1_SIZE_F Size() const noexcept { return (_Bitmap != nullptr) ? _Bitmap->GetSize() : D2D1::SizeF(); }

    void Render(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const State * state) noexcept;

    ID2D1Bitmap * Bitmap() const noexcept { return _Bitmap; }

    void Release() noexcept
    {
        _Bitmap.Release();
        _FormatConverter.Release();
        _Frame.Release();

        std::vector<uint8_t> Empty;

        _Raster.swap(Empty);

        SetIdle();
    }

    bool IsInitialized() const noexcept { return _Status == Initialized; }
    bool IsRealized() const noexcept { return _Status == Realized; }
    void SetIdle() noexcept { _Status = Idle; }
    void RequestColorUpdate() noexcept { _Status = Realized; } // Request an update of the color list because the parameters have changed.

private:
    CriticalSection _CriticalSection;

    std::vector<uint8_t> _Raster;
    std::wstring _FilePath;

    CComPtr<IWICBitmapFrameDecode> _Frame;
    CComPtr<IWICFormatConverter> _FormatConverter;
    CComPtr<ID2D1Bitmap> _Bitmap;

    enum ArtworkState
    {
        Idle = 0,

        Initialized,    // A new artwork source has been set.
        Realized,       // A new bitmap has been generated or the configuration parameters have changed.
    };

    ArtworkState _Status;
};

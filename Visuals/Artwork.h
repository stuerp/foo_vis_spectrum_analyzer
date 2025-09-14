
/** $VER: Artwork.h (2025.09.14) P. Stuer  **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>
#include <d2d1_2.h>

#include <libmsc.h>

#include "State.h"

#include "Log.h"

class Artwork
{
public:
    Artwork()
    {
        SetStatus(Idle);
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

    void Render(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const state_t * state) noexcept;

    ID2D1Bitmap * Bitmap() const noexcept { return _Bitmap; }

    void Release() noexcept
    {
    //  Log::Write(Log::Level::Trace, "%8d: Releasing artwork.", (uint32_t) ::GetTickCount64());

        _CriticalSection.Enter();

        _Bitmap.Release();
        _FormatConverter.Release();
        _Frame.Release();

        _FilePath.clear();

        std::vector<uint8_t> Empty;

        _Raster.swap(Empty);

        SetStatus(Idle);

        _CriticalSection.Leave();
    }

    bool IsIdle() const noexcept { return _Status == Idle; }
    bool IsInitialized() const noexcept { return _Status == Initialized; }
    bool IsRealized() const noexcept { return _Status == Realized; }

private:
    enum Status
    {
        Idle = 0,

        Initialized,    // A new artwork source has been set.
        Realized,       // A new bitmap has been generated or the configuration parameters have changed.
        GotColors,      // We've gotten the colors from the artwork.
    };

    void SetStatus(Status status) noexcept
    {
        _CriticalSection.Enter();

    //  Log::Write(Log::Level::Trace, "%8d: Setting artwork status to %d.", (uint32_t) ::GetTickCount64(), status);
        _Status = status;

        _CriticalSection.Leave();
    }

private:
    msc::critical_section_t _CriticalSection;

    std::vector<uint8_t> _Raster;
    std::wstring _FilePath;

    CComPtr<IWICBitmapFrameDecode> _Frame;
    CComPtr<IWICFormatConverter> _FormatConverter;
    CComPtr<ID2D1Bitmap> _Bitmap;

    Status _Status;
};

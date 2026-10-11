
/** $VER: DirectWrite.h (2026.10.04) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dwrite.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <mutex>

class DirectWriteFactory final
{
    typedef IDWriteFactory Interface;

public:
    DirectWriteFactory(const DirectWriteFactory &) = delete;
    DirectWriteFactory & operator=(const DirectWriteFactory &) = delete;

    [[nodiscard]]
    static ComPtr<Interface> Get() noexcept
    {
        return Instance()._Factory.Get();
    }

    static HRESULT Startup() noexcept
    {
        return Instance().Initialize();
    }

    static void Shutdown() noexcept
    {
        Instance().Terminate();
    }

private:
    DirectWriteFactory() noexcept = default;

    static DirectWriteFactory & Instance() noexcept
    {
        static DirectWriteFactory _Instance;

        return _Instance;
    }

    HRESULT Initialize() noexcept;
    void Terminate() noexcept;

private:
    static ComPtr<Interface> _Factory;
    static int64_t _ReferenceCount;

    std::mutex _Mutex;
};

#include <string>

class DirectWrite
{
public:
    static HRESULT CreateTextFormat(const std::wstring & fontFamilyName, FLOAT fontSize, DWRITE_TEXT_ALIGNMENT horizonalAlignment, DWRITE_PARAGRAPH_ALIGNMENT verticalAlignment, IDWriteTextFormat ** textFormat);
    static HRESULT GetTextMetrics(IDWriteTextFormat * textFormat, const std::wstring & text, FLOAT & width, FLOAT & height);
};

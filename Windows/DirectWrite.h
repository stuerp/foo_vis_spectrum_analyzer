
/** $VER: DirectWrite.h (2026.09.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dwrite_3.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <mutex>

class DirectWriteFactory final
{
public:
    DirectWriteFactory(const DirectWriteFactory &) = delete;
    DirectWriteFactory& operator=(const DirectWriteFactory &) = delete;

    [[nodiscard]]
    static ComPtr<IDWriteFactory3> Get() noexcept;

    static HRESULT Startup() noexcept;
    static void Shutdown() noexcept;

private:
    DirectWriteFactory() noexcept = default;

    static DirectWriteFactory & Instance() noexcept;

    HRESULT Initialize() noexcept;
    void Terminate() noexcept;

private:
    static ComPtr<IDWriteFactory3> _Factory;
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

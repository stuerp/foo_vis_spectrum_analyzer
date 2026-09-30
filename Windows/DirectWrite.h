
/** $VER: DirectWrite.h (2026.09.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dwrite_3.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <Win32Exception.h>

#include "DirectXFactory.h"

class DirectWriteFactory final : public DirectXFactory<DirectWriteFactory, IDWriteFactory3>
{
    friend class DirectXFactory<DirectWriteFactory, IDWriteFactory3>;

public:
    DirectWriteFactory(const DirectWriteFactory &) = delete;
    DirectWriteFactory & operator=(const DirectWriteFactory &) = delete;

private:
    DirectWriteFactory() = default;
    ~DirectWriteFactory() = default;

    void CreateFactory()
    {
        ComPtr<IDWriteFactory3> Factory;

        {
            HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(Factory), (IUnknown **) Factory.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                throw msc::win32_exception("Unable to create DirectWrite factory.", (DWORD) hr);
        }

        _Factory = std::move(Factory);
    }
};
/*
class DirectWriteFactory final
{
public:
    DirectWriteFactory(const DirectWriteFactory & ) = delete;
    DirectWriteFactory & operator=(const DirectWriteFactory &) = delete;

    [[nodiscard]]
    static ComPtr<IDWriteFactory3> Get()
    {
        return Instance()._Factory.Get();
    }

    static void Shutdown()
    {
        Instance()._Factory.Reset();
    }

private:
    DirectWriteFactory()
    {
        HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(_Factory), (IUnknown **) _Factory.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            throw msc::win32_exception("Unable to create DirectWrite factory.", (DWORD) hr);
    }

    static DirectWriteFactory & Instance()
    {
        static DirectWriteFactory Instance;

        return Instance;
    }

private:
    ComPtr<IDWriteFactory3> _Factory;
};
*/
#include <string>

class DirectWrite
{
public:
    static HRESULT CreateTextFormat(const std::wstring & fontFamilyName, FLOAT fontSize, DWRITE_TEXT_ALIGNMENT horizonalAlignment, DWRITE_PARAGRAPH_ALIGNMENT verticalAlignment, IDWriteTextFormat ** textFormat);
    static HRESULT GetTextMetrics(IDWriteTextFormat * textFormat, const std::wstring & text, FLOAT & width, FLOAT & height);
};

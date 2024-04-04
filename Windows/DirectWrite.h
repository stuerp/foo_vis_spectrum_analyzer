
/** $VER: DirectWrite.h (2024.03.27) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <dwrite.h>
#include <atlbase.h>
#include <string>

class DirectWrite
{
public:
    DirectWrite() { }

    HRESULT Initialize();
    void Terminate();

    HRESULT CreateTextFormat(const std::wstring & fontFamilyName, FLOAT fontSize, DWRITE_TEXT_ALIGNMENT horizonalAlignment, DWRITE_PARAGRAPH_ALIGNMENT verticalAlignment, CComPtr<IDWriteTextFormat> & textFormat) const noexcept;
    HRESULT GetTextMetrics(CComPtr<IDWriteTextFormat> & textFormat, const std::wstring & text, FLOAT & width, FLOAT & height) const noexcept;

public:
    CComPtr<IDWriteFactory> Factory;
};

extern DirectWrite _DirectWrite;


/** $VER: DirectWrite.cpp (2026.09.26) P. Stuer **/

#include "pch.h"

#include "DirectWrite.h"

#pragma comment(lib, "dwrite")

#pragma hdrstop

ComPtr<IDWriteFactory3> DirectWriteFactory::_Factory;
int64_t DirectWriteFactory::_ReferenceCount = 0;

ComPtr<IDWriteFactory3> DirectWriteFactory::Get() noexcept
{
    return Instance()._Factory.Get();
}

HRESULT DirectWriteFactory::Startup() noexcept
{
    return Instance().Initialize();
}

void DirectWriteFactory::Shutdown() noexcept
{
    Instance().Terminate();
}

DirectWriteFactory & DirectWriteFactory::Instance() noexcept
{
    static DirectWriteFactory _Instance;

    return _Instance;
}

HRESULT DirectWriteFactory::Initialize() noexcept
{
    std::lock_guard Lock(_Mutex);

    ++_ReferenceCount;

    if (_Factory)
        return S_OK;

    return ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(_Factory), (IUnknown **) _Factory.GetAddressOf());
}

void DirectWriteFactory::Terminate() noexcept
{
    std::lock_guard Lock(_Mutex);

    --_ReferenceCount;

    if (_ReferenceCount == 0)
        _Factory.Reset();
}

/// <summary>
/// Creates and initializes a TextFormat object.
/// </summary>
HRESULT DirectWrite::CreateTextFormat(const std::wstring & fontFamilyName, FLOAT fontSize, DWRITE_TEXT_ALIGNMENT horizonalAlignment, DWRITE_PARAGRAPH_ALIGNMENT verticalAlignment, IDWriteTextFormat ** textFormat)
{
    HRESULT hr = DirectWriteFactory::Get()->CreateTextFormat(fontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"", textFormat);

    if (SUCCEEDED(hr))
    {
        (*textFormat)->SetTextAlignment(horizonalAlignment);
        (*textFormat)->SetParagraphAlignment(verticalAlignment);
        (*textFormat)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }

    return hr;
}

/// <summary>
/// Gets metrics about the specified text.
/// </summary>
HRESULT DirectWrite::GetTextMetrics(IDWriteTextFormat * textFormat, const std::wstring & text, FLOAT & width, FLOAT & height)
{
    ComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = DirectWriteFactory::Get()->CreateTextLayout(text.c_str(), (UINT32) text.length(), textFormat, 100.f, 100.f, TextLayout.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        DWRITE_TEXT_METRICS TextMetrics = { };

        TextLayout->GetMetrics(&TextMetrics);

        // Calculate the metric.
        width  = ::ceil(TextMetrics.width);
        height = ::ceil(TextMetrics.height);
    }

    return hr;
}

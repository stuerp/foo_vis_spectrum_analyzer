
/** $VER: DirectWrite.cpp (2024.03.27) P. Stuer **/

#include "DirectWrite.h"

#include "COMException.h"

#pragma comment(lib, "dwrite")

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT DirectWrite::Initialize()
{
    HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(Factory), (IUnknown **) &Factory);

    if (!SUCCEEDED(hr))
        throw COMException(hr, L"Unable to create DirectWrite factory.");

    return hr;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void DirectWrite::Terminate()
{
    Factory.Release();
}

/// <summary>
/// Creates a TextFormat object.
/// </summary>
HRESULT DirectWrite::CreateTextFormat(const std::wstring & fontFamilyName, FLOAT fontSize, DWRITE_TEXT_ALIGNMENT horizonalAlignment, DWRITE_PARAGRAPH_ALIGNMENT verticalAlignment, CComPtr<IDWriteTextFormat> & textFormat) const noexcept
{
    HRESULT hr = Factory->CreateTextFormat(fontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"", &textFormat);

    if (SUCCEEDED(hr))
    {
        textFormat->SetTextAlignment(horizonalAlignment);
        textFormat->SetParagraphAlignment(verticalAlignment);
        textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }

    return hr;
}

/// <summary>
/// Gets metrics about the specified text.
/// </summary>
HRESULT DirectWrite::GetTextMetrics(CComPtr<IDWriteTextFormat> & textFormat, const std::wstring & text, FLOAT & width, FLOAT & height) const noexcept
{
    CComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = _DirectWrite.Factory->CreateTextLayout(text.c_str(), text.length(), textFormat, 100.f, 100.f, &TextLayout);

    if (SUCCEEDED(hr))
    {
        DWRITE_TEXT_METRICS TextMetrics = { };

        TextLayout->GetMetrics(&TextMetrics);

        // Calculate the metric.
        width  = TextMetrics.width;
        height = 2.f + TextMetrics.height + 2.f;
    }

    return hr;
}

DirectWrite _DirectWrite;

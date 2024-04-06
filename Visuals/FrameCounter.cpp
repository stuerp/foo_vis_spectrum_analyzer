
/** $VER: FrameCounter.cpp (2024.03.31) P. Stuer **/

#include "framework.h"
#include "FrameCounter.h"

#pragma hdrstop

/// <summary>
/// Registers the time when starting a new frame.
/// </summary>
void FrameCounter::NewFrame() noexcept
{
    LARGE_INTEGER Counter;

    ::QueryPerformanceCounter(&Counter);

    _Times.Add(Counter.QuadPart);
}

/// <summary>
/// Gets the current frame rate.
/// </summary>
float FrameCounter::GetFPS() const noexcept
{
    float FPS = (float)((_Times.Count() - 1) * _Frequency.QuadPart) / (float) (_Times.Last() - _Times.First());

    return FPS;
}

/// <summary>
/// Initializes this instance.
/// </summary>
void FrameCounter::Resize(FLOAT clientWidth, FLOAT clientHeight) noexcept
{
    _ClientWidth = clientWidth;
    _ClientHeight = clientHeight;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
HRESULT FrameCounter::Render(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    const FLOAT Inset = 4.f;

    static WCHAR Text[512] = { };

    if (SUCCEEDED(hr))
        hr = ::StringCchPrintfW(Text, _countof(Text), L"%.2f fps", GetFPS());

    if (SUCCEEDED(hr) && (_Brush != nullptr))
    {
        const D2D1_RECT_F Rect = { _ClientWidth - 2.f - (Inset + _TextWidth + Inset), 2.f, _ClientWidth - 2.f, 2.f + _TextHeight };

        // Draw the background.
        {
            _Brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f));

            renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _Brush);
        }

        // Draw the text.
        {
            _Brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

            renderTarget->DrawText(Text, (UINT) ::wcsnlen(Text, _countof(Text)), _TextFormat, Rect, _Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
    }

    return hr;
}

/// <summary>
/// Creates resources which are not bound to any D3D device.
/// </summary>
HRESULT FrameCounter::CreateDeviceIndependentResources() noexcept
{
    if (_TextFormat != nullptr)
        return S_OK;

    const FLOAT FontSize = ToDIPs(_FontSize); // In DIPs

    HRESULT hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

    if (SUCCEEDED(hr))
    {
        _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);            // Center horizontallly
        _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
        _TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        const WCHAR Text[] = L"999.99 fps";

        CComPtr<IDWriteTextLayout> TextLayout;

        hr = _DirectWrite.Factory->CreateTextLayout(Text, _countof(Text), _TextFormat, 1920.f, 1080.f, &TextLayout);

        if (SUCCEEDED(hr))
        {
            DWRITE_TEXT_METRICS TextMetrics = { };

            TextLayout->GetMetrics(&TextMetrics);

            _TextWidth  = TextMetrics.width;
            _TextHeight = TextMetrics.height;
        }
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void FrameCounter::ReleaseDeviceIndependentResources() noexcept
{
    _TextFormat.Release();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT FrameCounter::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    if (_Brush != nullptr)
        return S_OK;

    HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_Brush);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void FrameCounter::ReleaseDeviceSpecificResources() noexcept
{
    _Brush.Release();
}

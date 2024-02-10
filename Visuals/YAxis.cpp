
/** $VER: YAXis.cpp (2024.02.07) P. Stuer - Implements the Y axis of a graph. **/

#include "YAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void YAxis::Initialize(Configuration * configuration)
{
    _Configuration = configuration;

    _Labels.clear();

    if (_Configuration->_YAxisMode == YAxisMode::None)
        return;

    // Precalculate the labels and their position.
    {
        for (double Amplitude = _Configuration->_AmplitudeLo; Amplitude <= _Configuration->_AmplitudeHi; Amplitude -= _Configuration->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%ddB", (int) Amplitude);

            Label lb = { Amplitude, Text, 0.f };

            _Labels.push_back(lb);
        }
    }
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void YAxis::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;

    // Calculate the position of the labels based on the height.
    const FLOAT Height = _Bounds.bottom - _Bounds.top;

    for (Label & Iter : _Labels)
        Iter.y = Map(_Configuration->ScaleA(ToMagnitude(Iter.Amplitude)), 0.0, 1.0, Height, _Height / 2.f);
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void YAxis::Render(ID2D1RenderTarget * renderTarget)
{
    if (_Configuration->_YAxisMode == YAxisMode::None)
        return;

    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT Width = _Bounds.right - _Bounds.left;

    FLOAT OldTextTop = _Bounds.bottom - _Bounds.top + _Height;

    Style * Style1 = _Configuration->_StyleManager.GetStyle(VisualElement::YAxisLine);
    Style * Style2 = _Configuration->_StyleManager.GetStyle(VisualElement::YAxisText);

    for (const Label & Iter : _Labels)
    {
        // Draw the horizontal grid line.
        renderTarget->DrawLine(D2D1_POINT_2F(_Bounds.left + _Width, Iter.y), D2D1_POINT_2F(Width, Iter.y), Style1->_Brush, Style1->_Thickness, nullptr);

        // Draw the label.
        if (!Iter.Text.empty())
        {
            D2D1_RECT_F TextRect = { _Bounds.left, Iter.y - (_Height / 2.f), _Bounds.left + _Width - 2.f, Iter.y + (_Height / 2.f) };

            if (TextRect.bottom < OldTextTop)
            {
                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, Style2->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

                OldTextTop = TextRect.top;
            }
        }
    }
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT YAxis::CreateDeviceIndependentResources()
{
    static const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

    HRESULT hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

    if (SUCCEEDED(hr))
    {
        _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);          // Right-align horizontally
        _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically

        CComPtr<IDWriteTextLayout> TextLayout;

        hr = _DirectWrite.Factory->CreateTextLayout(L"AaGg09", 6, _TextFormat, 100.f, 100.f, &TextLayout);

        if (SUCCEEDED(hr))
        {
            DWRITE_TEXT_METRICS TextMetrics = { };

            TextLayout->GetMetrics(&TextMetrics);

            _Height = TextMetrics.height;
        }
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void YAxis::ReleaseDeviceIndependentResources()
{
    _TextFormat.Release();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT YAxis::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        for (const auto & Iter : { VisualElement::YAxisLine, VisualElement::YAxisText })
        {
            Style * style = _Configuration->_StyleManager.GetStyle(Iter);

            if (style->_Brush == nullptr)
                hr = style->CreateDeviceSpecificResources(renderTarget);

            if (!SUCCEEDED(hr))
                break;
        }
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void YAxis::ReleaseDeviceSpecificResources()
{
    for (const auto & Iter : { VisualElement::YAxisLine, VisualElement::YAxisText })
    {
        Style * style = _Configuration->_StyleManager.GetStyle(Iter);

        style->ReleaseDeviceSpecificResources();
    }
}

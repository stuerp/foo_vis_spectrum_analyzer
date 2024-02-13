
/** $VER: YAXis.cpp (2024.02.13) P. Stuer - Implements the Y axis of a graph. **/

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
    for (Label & Iter : _Labels)
        Iter.y = Map(_Configuration->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., _Bounds.bottom, _Bounds.top);
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

    const FLOAT xl = _Bounds.left  + (_Configuration->_YAxisLeft ?  _Width : 0.f); // Left axis
    const FLOAT xr = _Bounds.right - (_Configuration->_YAxisRight ? _Width : 0.f); // Right axis

    FLOAT OldTextTop = _Bounds.bottom + _Height;

    Style * LineStyle = _Configuration->_StyleManager.GetStyle(VisualElement::YAxisLine);
    Style * TextStyle = _Configuration->_StyleManager.GetStyle(VisualElement::YAxisText);

    for (const Label & Iter : _Labels)
    {
        // Draw the horizontal grid line.
        renderTarget->DrawLine(D2D1_POINT_2F(xl, Iter.y), D2D1_POINT_2F(xr, Iter.y), LineStyle->_Brush, LineStyle->_Thickness, nullptr);

        // Draw the label.
        if (!Iter.Text.empty())
        {
            FLOAT y = Iter.y - (_Height / 2.f);

            if (y <_Bounds.top)
                y = _Bounds.top;

            if (y + _Height > _Bounds.bottom)
                y = _Bounds.bottom - _Height;

            D2D1_RECT_F TextRect = { _Bounds.left, y, xl - 2.f, y + _Height };

            if (TextRect.bottom < OldTextTop)
            {
                if (_Configuration->_YAxisLeft)
                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

                if (_Configuration->_YAxisRight)
                {
                    TextRect.left  = xr + 2.f;
                    TextRect.right = _Bounds.right;

                    renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
                }

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

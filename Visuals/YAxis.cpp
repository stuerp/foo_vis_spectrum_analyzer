
/** $VER: YAXis.cpp (2023.12.31) P. Stuer - Implements the Y axis of a graph. **/

#include "YAxis.h"
#include "DirectX.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void YAxis::Initialize(const Configuration * configuration)
{
    _Configuration = configuration;

    _TextColor = configuration->_YTextColor;
    _LineColor = configuration->_YLineColor;

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
void YAxis::Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    if (_Configuration->_YAxisMode == YAxisMode::None)
        return;

    CreateDeviceSpecificResources(renderTarget);

    const FLOAT StrokeWidth = 1.0f;
    const FLOAT Width = _Bounds.right - _Bounds.left;

    FLOAT OldTextTop = _Bounds.bottom - _Bounds.top + _Height;

    for (const Label & Iter : _Labels)
    {
        // Draw the horizontal grid line.
        {
            _SolidBrush->SetColor(_Configuration->_UseCustomYLineColor ? _LineColor : ToD2D1_COLOR_F(_Configuration->_DefTextColor));

            renderTarget->DrawLine(D2D1_POINT_2F(_Bounds.left + _Width, Iter.y), D2D1_POINT_2F(Width, Iter.y), _SolidBrush, StrokeWidth, nullptr);
        }

        // Draw the label.
        {
            D2D1_RECT_F TextRect = { _Bounds.left, Iter.y - (_Height / 2.f), _Bounds.left + _Width - 2.f, Iter.y + (_Height / 2.f) };

            if (TextRect.bottom < OldTextTop)
            {
                _SolidBrush->SetColor(_Configuration->_UseCustomYTextColor ? _TextColor : ToD2D1_COLOR_F(_Configuration->_DefTextColor));

                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, _SolidBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);

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
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        static const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

        hr = _DirectX._DirectWrite->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);          // Right-align horizontally
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically
        }
    }

    if (SUCCEEDED(hr))
    {
        CComPtr<IDWriteTextLayout> TextLayout;

        hr = _DirectX._DirectWrite->CreateTextLayout(L"AaGg09", 6, _TextFormat, 100.f, 100.f, &TextLayout);

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
HRESULT YAxis::CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    if (_SolidBrush != nullptr)
        return S_OK;

    HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_SolidBrush);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void YAxis::ReleaseDeviceSpecificResources()
{
    _SolidBrush.Release();
}

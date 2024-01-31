
/** $VER: YAXis.cpp (2024.01.31) P. Stuer - Implements the Y axis of a graph. **/

#include "YAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

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
void YAxis::Render(ID2D1RenderTarget * renderTarget)
{
    if (_Configuration->_YAxisMode == YAxisMode::None)
        return;

    CreateDeviceSpecificResources(renderTarget);

    const FLOAT Width = _Bounds.right - _Bounds.left;

    FLOAT OldTextTop = _Bounds.bottom - _Bounds.top + _Height;

    for (const Label & Iter : _Labels)
    {
        // Draw the horizontal grid line.
        {
            Style & style = _StyleManager.GetStyle(VisualElement::YAxisLine);

            renderTarget->DrawLine(D2D1_POINT_2F(_Bounds.left + _Width, Iter.y), D2D1_POINT_2F(Width, Iter.y), style._Brush, style._Thickness, nullptr);
        }

        // Draw the label.
        {
            D2D1_RECT_F TextRect = { _Bounds.left, Iter.y - (_Height / 2.f), _Bounds.left + _Width - 2.f, Iter.y + (_Height / 2.f) };

            if (TextRect.bottom < OldTextTop)
            {
                Style & style = _StyleManager.GetStyle(VisualElement::YAxisText);

                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, TextRect, style._Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

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

    {
        Style & style = _StyleManager.GetStyle(VisualElement::YAxisLine);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::YAxisText);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void YAxis::ReleaseDeviceSpecificResources()
{
    {
        Style & style = _StyleManager.GetStyle(VisualElement::YAxisLine);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::YAxisText);

        style._Brush.Release();
    }
}

/// <summary>
/// Creates a gradient brush for rendering the bars.
/// </summary>
HRESULT YAxis::CreateGradientBrush(ID2D1RenderTarget * renderTarget, const GradientStops & gradientStops, ID2D1LinearGradientBrush ** gradientBrush)
{
    if (gradientStops.empty())
        return E_FAIL;

    CComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = renderTarget->CreateGradientStopCollection(&gradientStops[0], (UINT32) gradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F Size = renderTarget->GetSize();

        D2D1_POINT_2F Start = _Configuration->_HorizontalGradient ? D2D1::Point2F(       0.f, Size.height / 2.f) : D2D1::Point2F(Size.width / 2.f, 0.f);
        D2D1_POINT_2F End   = _Configuration->_HorizontalGradient ? D2D1::Point2F(Size.width, Size.height / 2.f) : D2D1::Point2F(Size.width / 2.f, Size.height);

        hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(Start, End), Collection, gradientBrush);
    }

    return hr;
}

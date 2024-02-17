
/** $VER: Graph.cpp (2024.02.17) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

#include "Graph.h"
#include "StyleManager.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Graph::Graph() : _Bounds(), _FontFamilyName(L"Segoe UI"), _FontSize(14.f)
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
void Graph::Initialize(State * state, uint32_t channels, const std::wstring & description) noexcept
{
    _State = state;
    _Description = description;

    _Analysis.Initialize(state, channels);

    _Spectrum.Initialize(state);

    _XAxis.Initialize(state, _Analysis);
    
    _YAxis.Initialize(state);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Graph::Move(const D2D1_RECT_F & rect) noexcept
{
    _Bounds = rect;

    const FLOAT xt = ((_State->_XAxisMode != XAxisMode::None) && _State->_XAxisTop)    ? _XAxis.GetHeight() : 0.f;
    const FLOAT xb = ((_State->_XAxisMode != XAxisMode::None) && _State->_XAxisBottom) ? _XAxis.GetHeight() : 0.f;

    const FLOAT yl = ((_State->_YAxisMode != YAxisMode::None) && _State->_YAxisLeft)   ? _YAxis.GetWidth()  : 0.f;
    const FLOAT yr = ((_State->_YAxisMode != YAxisMode::None) && _State->_YAxisRight)  ? _YAxis.GetWidth()  : 0.f;

    {
        D2D1_RECT_F Rect(_Bounds.left + yl, _Bounds.top + xt, _Bounds.right - yr, _Bounds.bottom - xb);

        _Spectrum.Move(Rect);
    }

    {
        D2D1_RECT_F Rect(_Bounds.left + yl, _Bounds.top,      _Bounds.right - yr, _Bounds.bottom);

        _XAxis.Move(Rect);
    }

    {
        D2D1_RECT_F Rect(_Bounds.left,      _Bounds.top + xt, _Bounds.right,      _Bounds.bottom - xb);

        _YAxis.Move(Rect);
    }
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void Graph::Render(ID2D1RenderTarget * renderTarget, double sampleRate, Artwork & artwork) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
    {
        RenderBackground(renderTarget, artwork);
        RenderForeground(renderTarget, _Analysis._FrequencyBands, sampleRate);
    }
}

/// <summary>
/// Clears the analysis of this instance.
/// </summary>
void Graph::Clear()
{
    for (FrequencyBand & fb : _Analysis._FrequencyBands)
        fb.CurValue = 0.;
}

/// <summary>
/// Renders the background.
/// </summary>
void Graph::RenderBackground(ID2D1RenderTarget * renderTarget, Artwork & artwork) noexcept
{
    const Style * style = _State->_StyleManager.GetStyle(VisualElement::Background);

    renderTarget->FillRectangle(_Bounds, style->_Brush);

    // Render the bitmap if there is one.
    if ((artwork.Bitmap() != nullptr) && (_State->_BackgroundMode == BackgroundMode::Artwork))
        artwork.Render(renderTarget, GetSpectrum().GetBounds(), _State);
}

/// <summary>
/// Renders the foreground.
/// </summary>
void Graph::RenderForeground(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) noexcept
{
    _XAxis.Render(renderTarget);

    _YAxis.Render(renderTarget);

    _Spectrum.Render(renderTarget, frequencyBands, sampleRate);

    RenderDescription(renderTarget);
}

/// <summary>
/// Renders the description.
/// </summary>
void Graph::RenderDescription(ID2D1RenderTarget * renderTarget) noexcept
{
    const Style * style = _State->_StyleManager.GetStyle(VisualElement::XAxisText);

    const FLOAT Inset = 2.f;

    D2D1_RECT_F Rect = { };

    Rect.left   = _Spectrum.GetBounds().left + 10.f;
    Rect.top    = _Spectrum.GetBounds().top + 10.f;
    Rect.right  = Rect.left + _TextWidth + (Inset * 2.f);
    Rect.bottom = Rect.top + _TextHeight + (Inset * 2.f);

    // FIXME
    FLOAT Opacity = style->_Brush->GetOpacity();

    style->_Brush->SetOpacity(Opacity * 0.25f);

    renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), style->_Brush);

    style->_Brush->SetOpacity(Opacity);

    renderTarget->DrawText(_Description.c_str(), (UINT) _Description.length(), _TextFormat, Rect, style->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Graph::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    if (_TextFormat == nullptr)
    {
        const FLOAT FontSize = ToDIPs(_FontSize); // In DIPs

        hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            CComPtr<IDWriteTextLayout> TextLayout;

            hr = _DirectWrite.Factory->CreateTextLayout(_Description.c_str(), _Description.length(), _TextFormat, _Bounds.right - _Bounds.left, _Bounds.bottom - _Bounds.top, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                DWRITE_OVERHANG_METRICS OverhangMetrics = { };

                TextLayout->GetOverhangMetrics(&OverhangMetrics);

                _TextWidth  = TextMetrics.width  + (OverhangMetrics.right - OverhangMetrics.left);
                _TextHeight = TextMetrics.height + (OverhangMetrics.bottom - OverhangMetrics.top);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        Style * style = _State->_StyleManager.GetStyle(VisualElement::Background);

        if (style->_Brush == nullptr)
            hr = style->CreateDeviceSpecificResources(renderTarget);

        style = _State->_StyleManager.GetStyle(VisualElement::XAxisText);

        if (style->_Brush == nullptr)
            hr = style->CreateDeviceSpecificResources(renderTarget);
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Graph::ReleaseDeviceSpecificResources() noexcept
{
    _Spectrum.ReleaseDeviceSpecificResources();
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();

    _State->_StyleManager.GetStyle(VisualElement::XAxisText)->ReleaseDeviceSpecificResources();
    _State->_StyleManager.GetStyle(VisualElement::Background)->ReleaseDeviceSpecificResources();

    _TextFormat.Release();
}

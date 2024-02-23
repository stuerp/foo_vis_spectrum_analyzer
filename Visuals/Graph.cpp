
/** $VER: Graph.cpp (2024.02.21) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

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
/// Destroys this instance.
/// </summary>
Graph::~Graph()
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
void Graph::Initialize(State * state, const GraphSettings * settings) noexcept
{
    _State = state;
    _GraphSettings = settings;

    _Description = pfc::wideFromUTF8(settings->_Description);

    _Analysis.Initialize(state, settings);

    _Spectrum.Initialize(state, settings);

    _XAxis.Initialize(state, settings, _Analysis._FrequencyBands);
    
    _YAxis.Initialize(state, settings);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Graph::Move(const D2D1_RECT_F & rect) noexcept
{
    _Bounds = rect;

    const FLOAT xt = ((_GraphSettings->_XAxisMode != XAxisMode::None) && _GraphSettings->_XAxisTop)    ? _XAxis.GetHeight() : 0.f;
    const FLOAT xb = ((_GraphSettings->_XAxisMode != XAxisMode::None) && _GraphSettings->_XAxisBottom) ? _XAxis.GetHeight() : 0.f;

    const FLOAT yl = ((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisLeft)   ? _YAxis.GetWidth()  : 0.f;
    const FLOAT yr = ((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisRight)  ? _YAxis.GetWidth()  : 0.f;

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
/// Gets the tool area of this graph.
/// </summary>
CToolInfo * Graph::GetToolInfo(HWND hParent) noexcept
{
    CToolInfo * ToolInfo = new CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, hParent, (UINT_PTR) hParent, nullptr, nullptr);

    ::SetRect(&ToolInfo->rect, (int) _Bounds.left, (int) _Bounds.top, (int) _Bounds.right, (int) _Bounds.bottom);

    return ToolInfo;
}

/// <summary>
/// Gets the tooltip at the specified x position.
/// </summary>
bool Graph::GetToolTip(FLOAT x, std::wstring & toolTip, size_t & index) const noexcept
{
    const D2D1_RECT_F & Bounds = _Spectrum.GetBounds();

    if (!InRange(x, Bounds.left, Bounds.right))
        return false;

    if (_GraphSettings->_FlipHorizontally)
        x = (Bounds.right + Bounds.left) - x;

    index = Clamp((size_t) ::floor(Map(x, Bounds.left, Bounds.right, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);

    toolTip = _Analysis._FrequencyBands[index].Label;

    return true;
}

/// <summary>
/// Renders the background.
/// </summary>
void Graph::RenderBackground(ID2D1RenderTarget * renderTarget, Artwork & artwork) noexcept
{
    renderTarget->FillRectangle(_Bounds, _BackgroundStyle->_Brush);

    // Render the bitmap if there is one.
    if ((artwork.Bitmap() != nullptr) && _State->_ShowArtworkOnBackground)
        artwork.Render(renderTarget, _Spectrum.GetBounds(), _State);
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
    if ((_DescriptionStyle->_ColorSource == ColorSource::None) || _Description.empty())
        return;

    const FLOAT Inset = 2.f;

    D2D1_RECT_F Rect = { };

    Rect.left   = _Spectrum.GetBounds().left + 10.f;
    Rect.top    = _Spectrum.GetBounds().top + 10.f;
    Rect.right  = Rect.left + _TextWidth + (Inset * 2.f);
    Rect.bottom = Rect.top + _TextHeight + (Inset * 2.f);

    FLOAT Opacity = _DescriptionStyle->_Brush->GetOpacity();

    _DescriptionStyle->_Brush->SetOpacity(Opacity * 0.25f);

    renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _DescriptionStyle->_Brush);

    _DescriptionStyle->_Brush->SetOpacity(Opacity);

    renderTarget->DrawText(_Description.c_str(), (UINT) _Description.length(), _TextFormat, Rect, _DescriptionStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
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

            hr = _DirectWrite.Factory->CreateTextLayout(_Description.c_str(), (UINT32) _Description.length(), _TextFormat, _Bounds.right - _Bounds.left, _Bounds.bottom - _Bounds.top, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                _TextWidth  = TextMetrics.width;
                _TextHeight = TextMetrics.height;
            }
        }
    }

    if (((_BackgroundStyle == nullptr) || (_DescriptionStyle == nullptr)) && SUCCEEDED(hr))
    {
        const D2D1_SIZE_F Size = renderTarget->GetSize();

        Style * style = _State->_StyleManager.GetStyle(VisualElement::GraphBackground);

        if (style->_Brush == nullptr)
            hr = style->CreateDeviceSpecificResources(renderTarget, Size);

        if (SUCCEEDED(hr))
            _BackgroundStyle = style;

        style = _State->_StyleManager.GetStyle(VisualElement::GraphDescription);

        if (style->_Brush == nullptr)
            hr = style->CreateDeviceSpecificResources(renderTarget, Size);

        if (SUCCEEDED(hr))
            _DescriptionStyle = style;
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

    _DescriptionStyle = nullptr;
    _BackgroundStyle = nullptr;

    for (const auto & Iter : { VisualElement::GraphBackground, VisualElement::GraphDescription })
    {
        Style * style = _State->_StyleManager.GetStyle(Iter);

        style->ReleaseDeviceSpecificResources();
    }

    _TextFormat.Release();
}

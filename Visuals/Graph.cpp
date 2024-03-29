
/** $VER: Graph.cpp (2024.03.17) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

#include "Graph.h"
#include "StyleManager.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Graph::Graph() : _Bounds()
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

    _Description = settings->_Description;

    _Analysis.Initialize(state, settings);

    _Spectrum.Initialize(state, settings);

    _XAxis.Initialize(state, settings, _Analysis._FrequencyBands);
    
    _YAxis.Initialize(state, settings);

    _Spectogram.Initialize(state, settings, _Analysis._FrequencyBands);
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

    {
        _Spectogram.Move(_Bounds);
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
/// Resets this instance.
/// </summary>
void Graph::Reset()
{
    for (FrequencyBand & fb : _Analysis._FrequencyBands)
        fb.CurValue = 0.;

    _Spectogram.Reset();
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
//  if (_BackgroundStyle->_ColorSource != ColorSource::None)
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
    if (_State->_VisualizationType != VisualizationType::Spectogram)
    {
        _XAxis.Render(renderTarget);

        _YAxis.Render(renderTarget);

        _Spectrum.Render(renderTarget, frequencyBands, sampleRate);
    }
    else
        _Spectogram.Render(renderTarget, frequencyBands, sampleRate);

    RenderDescription(renderTarget);
}

/// <summary>
/// Renders the description.
/// </summary>
void Graph::RenderDescription(ID2D1RenderTarget * renderTarget) noexcept
{
    if (_Description.empty())
        return;

    CComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = _DirectWrite.Factory->CreateTextLayout(_Description.c_str(), (UINT32) _Description.length(), _DescriptionTextStyle->_TextFormat, _Bounds.right - _Bounds.left, _Bounds.bottom - _Bounds.top, &TextLayout);

    DWRITE_TEXT_METRICS TextMetrics = { };

    if (SUCCEEDED(hr))
        hr = TextLayout->GetMetrics(&TextMetrics);

    if (SUCCEEDED(hr))
    {
        const FLOAT Inset = 2.f;

        D2D1_RECT_F Rect = { };

        Rect.left   = _Spectrum.GetBounds().left + 10.f;
        Rect.top    = _Spectrum.GetBounds().top  + 10.f;
        Rect.right  = Rect.left + TextMetrics.width  + (Inset * 2.f);
        Rect.bottom = Rect.top  + TextMetrics.height + (Inset * 2.f);

        if (_DescriptionBackgroundStyle->_ColorSource != ColorSource::None)
            renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _DescriptionBackgroundStyle->_Brush);

        if (_DescriptionTextStyle->_ColorSource != ColorSource::None)
            renderTarget->DrawText(_Description.c_str(), (UINT) _Description.length(), _DescriptionTextStyle->_TextFormat, Rect, _DescriptionTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Graph::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    const D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
    {
        if (_BackgroundStyle == nullptr)
            _BackgroundStyle = _State->_StyleManager.GetStyle(VisualElement::GraphBackground);

        if (_BackgroundStyle && (_BackgroundStyle->_Brush == nullptr))
            hr = _BackgroundStyle->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_DescriptionTextStyle == nullptr)
            _DescriptionTextStyle = _State->_StyleManager.GetStyle(VisualElement::GraphDescriptionText);

        if (_DescriptionTextStyle && (_DescriptionTextStyle->_Brush == nullptr))
            hr = _DescriptionTextStyle->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_DescriptionBackgroundStyle == nullptr)
            _DescriptionBackgroundStyle = _State->_StyleManager.GetStyle(VisualElement::GraphDescriptionBackground);

        if (_DescriptionBackgroundStyle && (_DescriptionBackgroundStyle->_Brush == nullptr))
            hr = _DescriptionBackgroundStyle->CreateDeviceSpecificResources(renderTarget, Size);
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

    _DescriptionBackgroundStyle = nullptr;
    _DescriptionTextStyle = nullptr;
    _BackgroundStyle = nullptr;

    for (const auto & Iter : { VisualElement::GraphBackground, VisualElement::GraphDescriptionText, VisualElement::GraphDescriptionBackground })
    {
        Style * style = _State->_StyleManager.GetStyle(Iter);

        style->ReleaseDeviceSpecificResources();
    }
}

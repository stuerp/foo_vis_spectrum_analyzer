
/** $VER: Graph.cpp (2024.03.30) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

#include "Graph.h"
#include "StyleManager.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Graph::Graph() : _Bounds(), _Size()
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

    _PeakMeter.Initialize(state, settings);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Graph::Move(const D2D1_RECT_F & rect) noexcept
{
    _Bounds = rect;
    _Size = { rect.right - rect.left, rect.bottom - rect.top };

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

    {
        _PeakMeter.Move(_Bounds);
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
/// Initializes a structure with the tool area of this graph.
/// </summary>
void Graph::InitToolInfo(HWND hWnd, TTTOOLINFOW & ti) const noexcept
{
    ti = CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, hWnd, (UINT_PTR) hWnd, nullptr, nullptr);

    ::SetRect(&ti.rect, (int) _Bounds.left, (int) _Bounds.top, (int) _Bounds.right, (int) _Bounds.bottom);
}

/// <summary>
/// Gets the tooltip at the specified x or y position.
/// </summary>
bool Graph::GetToolTipText(FLOAT x, FLOAT y, std::wstring & toolTip, size_t & index) const noexcept
{
    if ((_State->_VisualizationType == VisualizationType::Bars) || (_State->_VisualizationType == VisualizationType::Curve))
    {
        const D2D1_RECT_F & Bounds = _Spectrum.GetBounds();

        const FLOAT Bandwidth = Max(::floor((Bounds.right - Bounds.left) / (FLOAT) _Analysis._FrequencyBands.size()), 2.f);

        const FLOAT SpectrumWidth = (_State->_VisualizationType == VisualizationType::Bars) ? Bandwidth * (FLOAT) _Analysis._FrequencyBands.size() : Bounds.right - Bounds.left;

        const FLOAT x1 = Bounds.left + ((Bounds.right - Bounds.left) - SpectrumWidth) / 2.f;
        const FLOAT x2 = x1 + SpectrumWidth;

        if (!InRange(x, x1, x2))
            return false;

        if (_GraphSettings->_FlipHorizontally)
            x = (x2 + x1) - x;

        index = Clamp((size_t) ::floor(Map(x, x1, x2, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
    }
    else
    if (_State->_VisualizationType == VisualizationType::Spectogram)
    {
        const D2D1_RECT_F & Bounds = _Spectogram.GetBounds();

        if (!InRange(y, Bounds.top, Bounds.bottom))
            return false;

        if (!_GraphSettings->_FlipVertically)
            y = (Bounds.bottom + Bounds.top) - y;

        index = Clamp((size_t) ::floor(Map(y, Bounds.top, Bounds.bottom, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
    }
    else
        return false;

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
    if ((artwork.Bitmap() != nullptr) && _State->_ShowArtworkOnBackground && (_State->_VisualizationType != VisualizationType::PeakMeter))
        artwork.Render(renderTarget, _Spectrum.GetBounds(), _State);
}

/// <summary>
/// Renders the foreground.
/// </summary>
void Graph::RenderForeground(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) noexcept
{
    switch (_State->_VisualizationType)
    {
        case VisualizationType::Bars:
        case VisualizationType::Curve:
        {
            _XAxis.Render(renderTarget);

            _YAxis.Render(renderTarget);

            _Spectrum.Render(renderTarget, frequencyBands, sampleRate);
            RenderDescription(renderTarget);
            break;
        }

        case VisualizationType::Spectogram:
        {
            _Spectogram.Render(renderTarget, frequencyBands, sampleRate);
            RenderDescription(renderTarget);
            break;
        }

        case VisualizationType::PeakMeter:
        {
            _PeakMeter.Render(renderTarget, _Analysis);
            break;
        }
    }
}

/// <summary>
/// Renders the description.
/// </summary>
void Graph::RenderDescription(ID2D1RenderTarget * renderTarget) noexcept
{
    if (_Description.empty())
        return;

    CComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = _DirectWrite.Factory->CreateTextLayout(_Description.c_str(), (UINT32) _Description.length(), _DescriptionTextStyle->_TextFormat, _Size.width, _Size.height, &TextLayout);

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
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphBackground, renderTarget, Size, &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphDescriptionText, renderTarget, Size, &_DescriptionTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphDescriptionBackground, renderTarget, Size, &_DescriptionBackgroundStyle);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Graph::ReleaseDeviceSpecificResources() noexcept
{
    _PeakMeter.ReleaseDeviceSpecificResources();

    _Spectogram.ReleaseDeviceSpecificResources();

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


/** $VER: Graph.cpp (2024.02.15) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

#include "Graph.h"
#include "StyleManager.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Graph::Graph() : _Bounds()
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
void Graph::Initialize(State * state, const std::vector<FrequencyBand> & frequencyBands)
{
    _State = state;

    _Spectrum.Initialize(state);

    _XAxis.Initialize(state, frequencyBands);
    
    _YAxis.Initialize(state);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Graph::Move(const D2D1_RECT_F & rect)
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
void Graph::Render(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate, const Artwork & artwork)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
    {
        RenderBackground(renderTarget, artwork);
        RenderForeground(renderTarget, frequencyBands, sampleRate);
    }
}

/// <summary>
/// Renders the background.
/// </summary>
void Graph::RenderBackground(ID2D1RenderTarget * renderTarget, const Artwork & artwork)
{
    const Style * style = _State->_StyleManager.GetStyle(VisualElement::Background);

    if (style->_ColorSource != ColorSource::Gradient)
        renderTarget->Clear(style->_Color);
    else
        renderTarget->FillRectangle(_Bounds, style->_Brush);

    // Render the bitmap if there is one.
    if ((artwork.Bitmap() != nullptr) && (_State->_BackgroundMode == BackgroundMode::Artwork))
        artwork.Render(renderTarget, GetSpectrum().GetBounds(), _State);
}

/// <summary>
/// Renders the foreground.
/// </summary>
void Graph::RenderForeground(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    _XAxis.Render(renderTarget);

    _YAxis.Render(renderTarget);

    _Spectrum.Render(renderTarget, frequencyBands, sampleRate);
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT Graph::CreateDeviceIndependentResources()
{
    HRESULT hr = _XAxis.CreateDeviceIndependentResources();

    if (SUCCEEDED(hr))
        hr = _YAxis.CreateDeviceIndependentResources();

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void Graph::ReleaseDeviceIndependentResources()
{
    _YAxis.ReleaseDeviceIndependentResources();
    _XAxis.ReleaseDeviceIndependentResources();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Graph::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    Style * style = _State->_StyleManager.GetStyle(VisualElement::Background);

    if (style->_Brush == nullptr)
        hr = style->CreateDeviceSpecificResources(renderTarget);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Graph::ReleaseDeviceSpecificResources()
{
    _Spectrum.ReleaseDeviceSpecificResources();
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();

    _State->_StyleManager.GetStyle(VisualElement::Background)->ReleaseDeviceSpecificResources();
}

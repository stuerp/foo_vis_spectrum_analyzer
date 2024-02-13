
/** $VER: Graph.cpp (2024.02.07) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

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
void Graph::Initialize(Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands)
{
    _Configuration = configuration;

    _Spectrum.Initialize(configuration);

    _XAxis.Initialize(configuration, frequencyBands);
    
    _YAxis.Initialize(configuration);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Graph::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;

    const FLOAT dy1 = (_Configuration->_YAxisMode != YAxisMode::None) ? _YAxis.GetWidth()  : 0.f;
    const FLOAT dy2 = (_Configuration->_RightYAxis ? dy1 : 0.f);

    const FLOAT dx1 = (_Configuration->_XAxisMode != XAxisMode::None) ? _XAxis.GetHeight() : 0.f;
    const FLOAT dx2 = (_Configuration->_TopXAxis ? dx1 : 0.f);

    {
        D2D1_RECT_F Rect(dy1, dx2, _Bounds.right - dy2, _Bounds.bottom - dx1);

        _Spectrum.Move(Rect);
    }

    {
        D2D1_RECT_F Rect(dy1, 0.f, _Bounds.right - dy2, _Bounds.bottom);

        _XAxis.Move(Rect);
    }

    {
        D2D1_RECT_F Rect(0.f, dx2, _Bounds.right, _Bounds.bottom - dx1);

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
    const Style * style = _Configuration->_StyleManager.GetStyle(VisualElement::Background);

    if (style->_ColorSource != ColorSource::Gradient)
        renderTarget->Clear(style->_Color);
    else
        renderTarget->FillRectangle(_Bounds, style->_Brush);

    // Render the album art if there is any.
    if ((artwork.Bitmap() == nullptr) || (_Configuration->_BackgroundMode != BackgroundMode::Artwork))
        return;

    D2D1_SIZE_F Size = artwork.Size();
    D2D1_RECT_F Rect = _Bounds;

    FLOAT MaxWidth  = Rect.right  - Rect.left;
    FLOAT MaxHeight = Rect.bottom - Rect.top;

    // Fit big images (Free / FitBig / FitWidth / FitHeight)
    {
        // Fit big images.
        FLOAT HScalar = (Size.width  > MaxWidth)  ? (FLOAT) MaxWidth  / (FLOAT) Size.width  : 1.f;
        FLOAT VScalar = (Size.height > MaxHeight) ? (FLOAT) MaxHeight / (FLOAT) Size.height : 1.f;

        FLOAT Scalar = (std::min)(HScalar, VScalar);

        Size.width  *= Scalar;
        Size.height *= Scalar;
    }

    Rect.left   = (MaxWidth  - Size.width)  / 2.f;
    Rect.top    = (MaxHeight - Size.height) / 2.f;
    Rect.right  = Rect.left + Size.width;
    Rect.bottom = Rect.top  + Size.height;

    renderTarget->DrawBitmap(artwork.Bitmap(), Rect, _Configuration->_ArtworkOpacity, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
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

    Style * style = _Configuration->_StyleManager.GetStyle(VisualElement::Background);

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

    _Configuration->_StyleManager.GetStyle(VisualElement::Background)->ReleaseDeviceSpecificResources();
}

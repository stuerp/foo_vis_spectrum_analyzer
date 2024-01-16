
/** $VER: Graph.cpp (2024.01.16) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

#include "Graph.h"

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
void Graph::Initialize(const Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands)
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

    const FLOAT dw = (_Configuration->_YAxisMode != YAxisMode::None) ? _YAxis.GetWidth()  : 0.f;
    const FLOAT dh = (_Configuration->_XAxisMode != XAxisMode::None) ? _XAxis.GetHeight() : 0.f;

    {
        D2D1_RECT_F Rect(dw, 0.f, _Bounds.right, _Bounds.bottom - dh);

        _Spectrum.Move(Rect);
    }

    {
        D2D1_RECT_F Rect(dw, 0.f, _Bounds.right, _Bounds.bottom);

        _XAxis.Move(Rect);
    }

    {
        D2D1_RECT_F Rect(0.f, 0.f, _Bounds.right, _Bounds.bottom - dh);

        _YAxis.Move(Rect);
    }
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void Graph::Render(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
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

HRESULT Graph::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    return S_OK;
}

void Graph::ReleaseDeviceSpecificResources()
{
    _Spectrum.ReleaseDeviceSpecificResources();
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();
}

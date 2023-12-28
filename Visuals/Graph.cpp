
/** $VER: Graph.cpp (2023.12.28) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Graph.h"

#pragma hdrstop

Graph::Graph() : _Bounds()
{
}

void Graph::Initialize(const Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands, CComPtr<ID2D1Factory> & direct2dFactory)
{
    _Configuration = configuration;

    _Spectrum.Initialize(configuration, direct2dFactory);

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
void Graph::Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    _XAxis.Render(renderTarget);

    _YAxis.Render(renderTarget);

    _Spectrum.Render(renderTarget, frequencyBands, sampleRate);
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT Graph::CreateDeviceIndependentResources(CComPtr<IDWriteFactory> & directWriteFactory)
{
    HRESULT hr = _XAxis.CreateDeviceIndependentResources(directWriteFactory);

    if (SUCCEEDED(hr))
        hr = _YAxis.CreateDeviceIndependentResources(directWriteFactory);

    return hr;
}

HRESULT Graph::CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    return S_OK;
}

void Graph::ReleaseDeviceSpecificResources()
{
    _Spectrum.ReleaseDeviceSpecificResources();
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();
}

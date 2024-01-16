
/** $VER: Graph.h (2024.01.16) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

#pragma once

#include "framework.h"
#include "Support.h"
#include "Configuration.h"

#include "Spectrum.h"
#include "XAxis.h"
#include "YAxis.h"

/// <summary>
/// Implements a graphical represenation of the spectrum analysis.
/// </summary>
class Graph
{
public:
    Graph();
    virtual ~Graph() { }

    void Initialize(const Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands);

    void Move(const D2D1_RECT_F & rect);

    void Render(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate);

    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    const D2D1_RECT_F & GetBounds() const { return _Bounds; }

    FLOAT GetLeft() const { return _Bounds.left; }
    FLOAT GetRight() const { return _Bounds.right; }

    Spectrum & GetSpectrum() { return _Spectrum; }

private:
    D2D1_RECT_F _Bounds;

    const Configuration * _Configuration;

    Spectrum _Spectrum;
    XAxis _XAxis;
    YAxis _YAxis;
};

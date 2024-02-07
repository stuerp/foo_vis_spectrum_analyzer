
/** $VER: Graph.h (2024.02.05) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

#pragma once

#include "framework.h"
#include "Support.h"
#include "Configuration.h"
#include "Artwork.h"

#include "Element.h"
#include "Spectrum.h"
#include "XAxis.h"
#include "YAxis.h"

/// <summary>
/// Implements a graphical representation of the spectrum analysis.
/// </summary>
class Graph : public Element
{
public:
    Graph();
    virtual ~Graph() { }

    void Initialize(Configuration * configuration, const std::vector<FrequencyBand> & frequencyBands);

    void Move(const D2D1_RECT_F & rect);

    void Render(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate, const Artwork & artwork);

    HRESULT CreateDeviceIndependentResources();
    void ReleaseDeviceIndependentResources();

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget);
    void ReleaseDeviceSpecificResources();

    const D2D1_RECT_F & GetBounds() const { return _Bounds; }

    FLOAT GetLeft() const { return _Bounds.left; }
    FLOAT GetRight() const { return _Bounds.right; }

    Spectrum & GetSpectrum() { return _Spectrum; }

private:
    void RenderForeground(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate);
    void RenderBackground(ID2D1RenderTarget * renderTarget, const Artwork & artwork);

private:
    D2D1_RECT_F _Bounds;

    Spectrum _Spectrum;
    XAxis _XAxis;
    YAxis _YAxis;
};

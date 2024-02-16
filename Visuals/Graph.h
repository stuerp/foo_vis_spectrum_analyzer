
/** $VER: Graph.h (2024.02.16) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

#pragma once

#include "framework.h"
#include "Support.h"
#include "State.h"
#include "Artwork.h"
#include "Analysis.h"

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

    void Initialize(State * state, Analyses & analyses) noexcept;

    void Move(const D2D1_RECT_F & rect) noexcept;

    void Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate, const Artwork & artwork) noexcept;

    const D2D1_RECT_F & GetBounds() const noexcept { return _Bounds; }

    FLOAT GetLeft() const noexcept { return _Bounds.left; }
    FLOAT GetRight() const noexcept { return _Bounds.right; }

    Spectrum & GetSpectrum() noexcept { return _Spectrum; }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    void RenderForeground(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) noexcept;
    void RenderBackground(ID2D1RenderTarget * renderTarget, const Artwork & artwork) noexcept;

    void MoveVisuals(const D2D1_RECT_F & rect) noexcept;

private:
    D2D1_RECT_F _Bounds;

    Spectrum _Spectrum;
    XAxis _XAxis;
    YAxis _YAxis;
};

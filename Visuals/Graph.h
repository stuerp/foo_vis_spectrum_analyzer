
/** $VER: Graph.h (2024.02.17) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

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

    void Initialize(State * state, uint32_t channels, const std::wstring & description) noexcept;

    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget, double sampleRate, const Artwork & artwork) noexcept;
    void Clear();

    void Process(const audio_chunk & chunk) noexcept
    {
        _Analysis.Process(chunk);
    }

    const D2D1_RECT_F & GetBounds() const noexcept { return _Bounds; }
    Analysis & GetAnalysis() noexcept { return _Analysis; }

    FLOAT GetLeft() const noexcept { return _Bounds.left; }
    FLOAT GetRight() const noexcept { return _Bounds.right; }

    Spectrum & GetSpectrum() noexcept { return _Spectrum; }

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    void RenderForeground(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) noexcept;
    void RenderBackground(ID2D1RenderTarget * renderTarget, const Artwork & artwork) noexcept;

    void RenderDescription(ID2D1RenderTarget * renderTarget) noexcept;

private:
    D2D1_RECT_F _Bounds;
    std::wstring _Description;

    Analysis _Analysis;

    Spectrum _Spectrum;
    XAxis _XAxis;
    YAxis _YAxis;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    CComPtr<IDWriteTextFormat> _TextFormat;
    FLOAT _TextWidth;
    FLOAT _TextHeight;
};

typedef std::vector<Graph *> Graphs;

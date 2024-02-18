
/** $VER: Graph.h (2024.02.18) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

#pragma once

#include "framework.h"
#include "Support.h"
#include "State.h"
#include "Artwork.h"
#include "Analysis.h"
#include "GraphSettings.h"

#include "Element.h"
#include "Spectrum.h"
#include "XAxis.h"
#include "YAxis.h"

#include "Log.h"

/// <summary>
/// Implements a graphical representation of the spectrum analysis.
/// </summary>
class Graph : public Element
{
public:
    Graph();
    virtual ~Graph();

    void Initialize(State * state, const GraphSettings & settings) noexcept;

    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget, double sampleRate, Artwork & artwork) noexcept;
    void Clear();

    void Process(const audio_chunk & chunk) noexcept
    {
        _Analysis.Process(chunk);
    }

    const D2D1_RECT_F & GetBounds() const noexcept { return _Bounds; }

    FLOAT GetLeft() const noexcept { return _Bounds.left; }
    FLOAT GetRight() const noexcept { return _Bounds.right; }

    Analysis & GetAnalysis() noexcept { return _Analysis; }
    Spectrum & GetSpectrum() noexcept { return _Spectrum; }

    CToolInfo * GetToolInfo(HWND hParent) noexcept;

    /// <summary>
    /// Returns true if the specified points lies with our bounds.
    /// </summary>
    bool ContainsPoint(const CPoint & pt) const noexcept
    {
        const D2D1_RECT_F & Bounds = _Spectrum.GetBounds();

        if ((FLOAT) pt.x < Bounds.left)
            return false;

        if ((FLOAT) pt.x > Bounds.right)
            return false;

        if ((FLOAT) pt.y < Bounds.top)
            return false;

        if ((FLOAT) pt.y > Bounds.bottom)
            return false;

        return true;
    }

    bool GetToolTip(FLOAT x, std::wstring & toolTip, size_t & index) const noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    void RenderForeground(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) noexcept;
    void RenderBackground(ID2D1RenderTarget * renderTarget, Artwork & artwork) noexcept;

    void RenderDescription(ID2D1RenderTarget * renderTarget) noexcept;

private:
    std::wstring _Description;

    D2D1_RECT_F _Bounds;

    Analysis _Analysis;

    Spectrum _Spectrum;
    XAxis _XAxis;
    YAxis _YAxis;

    std::wstring _FontFamilyName;
    FLOAT _FontSize;    // In points.

    CComPtr<IDWriteTextFormat> _TextFormat;
    FLOAT _TextWidth;
    FLOAT _TextHeight;

    Style * _BackgroundStyle;
    Style * _DescriptionStyle;
};

typedef std::vector<Graph *> Graphs;

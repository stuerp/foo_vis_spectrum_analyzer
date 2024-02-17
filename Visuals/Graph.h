
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
    virtual ~Graph();

    void Initialize(State * state, uint32_t channels, const std::wstring & description) noexcept;

    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget, double sampleRate, Artwork & artwork) noexcept;
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

    CToolInfo * GetToolInfo(HWND hParent) noexcept;

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

    size_t GetToolTip(FLOAT x, std::wstring & toolTip)
    {
        const D2D1_RECT_F & Bounds = _Spectrum.GetBounds();

        if ((x < Bounds.left) || (Bounds.right < x))
            return ~0U;

        size_t Index = (size_t) ::floor(Map(x, Bounds.left, Bounds.right, 0., (double) _Analysis._FrequencyBands.size() - 1.));

        toolTip = _Analysis._FrequencyBands[Index].Label;

        return Index;
    }

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
};

typedef std::vector<Graph *> Graphs;

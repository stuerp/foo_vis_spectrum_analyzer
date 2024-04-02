
/** $VER: Graph.h (2024.04.02) P. Stuer - Implements a graphical representation of a spectrum analysis. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Support.h"
#include "State.h"
#include "Artwork.h"
#include "Analysis.h"
#include "GraphSettings.h"

#include "Element.h"

#include "Spectrum.h"
#include "Spectogram.h"
#include "PeakMeter.h"

#include "Log.h"

/// <summary>
/// Implements a graphical representation of the spectrum analysis.
/// </summary>
class Graph : public Element
{
public:
    Graph();
    virtual ~Graph();

    void Initialize(State * state, const GraphSettings * settings) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget, double sampleRate, Artwork & artwork) noexcept;
    void Reset();

    void Process(const audio_chunk & chunk) noexcept
    {
        _Analysis.Process(chunk);
    }

    Analysis & GetAnalysis() noexcept { return _Analysis; }
    Spectrum & GetSpectrum() noexcept { return _Spectrum; }
    Spectogram & GetSpectogram() noexcept { return _Spectogram; }
    PeakMeter & GetPeakMeter() noexcept { return _PeakMeter; }

    void InitToolInfo(HWND hParent, TTTOOLINFOW & ti) const noexcept;

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

    bool GetToolTipText(FLOAT x, FLOAT y, std::wstring & toolTip, size_t & index) const noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    void RenderForeground(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) noexcept;
    void RenderBackground(ID2D1RenderTarget * renderTarget, Artwork & artwork) noexcept;

    void RenderDescription(ID2D1RenderTarget * renderTarget) noexcept;

private:
    std::wstring _Description;

    Analysis _Analysis;

    Spectrum _Spectrum;
    Spectogram _Spectogram;
    PeakMeter _PeakMeter;

    Style * _BackgroundStyle;
    Style * _DescriptionTextStyle;
    Style * _DescriptionBackgroundStyle;
};

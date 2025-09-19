
/** $VER: Graph.h (2024.04.22) P. Stuer - Implements a graphical representation of the spectrum analysis. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Support.h"
#include "Artwork.h"

#include "Element.h"

#include "Spectrum.h"
#include "Spectogram.h"
#include "PeakMeter.h"
#include "LevelMeter.h"

#include "Log.h"

/// <summary>
/// Implements a graphical representation of the spectrum analysis.
/// </summary>
class graph_t : public element_t
{
public:
    graph_t();
    virtual ~graph_t();

    void Initialize(state_t * state, const graph_settings_t * settings) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1RenderTarget * renderTarget, artwork_t & artwork) noexcept;
    void Reset();

    void Process(const audio_chunk & chunk) noexcept
    {
        _Analysis.Process(chunk);
    }

    void InitToolInfo(HWND hParent, TTTOOLINFOW & ti) const noexcept;

    /// <summary>
    /// Returns true if the specified point lies within our bounds.
    /// </summary>
    bool ContainsPoint(const CPoint & pt) const noexcept
    {
        if ((FLOAT) pt.x < _Bounds.left)
            return false;

        if ((FLOAT) pt.x > _Bounds.right)
            return false;

        if ((FLOAT) pt.y < _Bounds.top)
            return false;

        if ((FLOAT) pt.y > _Bounds.bottom)
            return false;

        return true;
    }

    bool GetToolTipText(FLOAT x, FLOAT y, std::wstring & toolTip, size_t & index) const noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    void RenderBackground(ID2D1RenderTarget * renderTarget, artwork_t & artwork) noexcept;
    void RenderForeground(ID2D1RenderTarget * renderTarget) noexcept;
    void RenderDescription(ID2D1RenderTarget * renderTarget) noexcept;

public:
    analysis_t _Analysis;

private:
    std::wstring _Description;

    spectrum_t _Spectrum;
    spectogram_t _Spectogram;
    peak_meter_t _PeakMeter;
    level_meter_t _LevelMeter;

    style_t * _BackgroundStyle;
    style_t * _DescriptionTextStyle;
    style_t * _DescriptionBackgroundStyle;
};

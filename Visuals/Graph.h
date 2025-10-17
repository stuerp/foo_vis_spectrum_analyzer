
/** $VER: Graph.h (2025.10.14) P. Stuer - Implements a graph on which the visualizations are rendered. **/

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
#include "Oscilloscope.h"
#include "OscilloscopeXY.h"

#include "Log.h"

/// <summary>
/// Implements a graph on which the visual are rendered.
/// </summary>
class graph_t : public element_t
{
public:
    graph_t();
    virtual ~graph_t();

    void Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept;
    void Move(const D2D1_RECT_F & rect) noexcept;
    void Render(ID2D1DeviceContext * deviceContext) noexcept { };
    void Reset() noexcept;

    void Process(const audio_chunk & chunk) noexcept;
    void Render(ID2D1DeviceContext * deviceContext, artwork_t & artwork) noexcept;

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

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void ReleaseDeviceSpecificResources() noexcept;

private:
    void RenderBackground(ID2D1DeviceContext * deviceContext, artwork_t & artwork) noexcept;
    void RenderForeground(ID2D1DeviceContext * deviceContext) noexcept;
    void RenderDescription(ID2D1DeviceContext * deviceContext) noexcept;

public:
    analysis_t _Analysis;

private:
    std::wstring _Description;
    std::unique_ptr<element_t> _Visualization;

    style_t * _BackgroundStyle;
    style_t * _DescriptionTextStyle;
    style_t * _DescriptionBackgroundStyle;

#ifdef _DEBUG
    CComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

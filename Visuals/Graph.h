
/** $VER: Graph.h (2026.10.24) P. Stuer - Implements a graph on which the visualizations are rendered. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Artwork.h"
#include "Element.h"
#include "RenderContext.h"
#include "Visualization.h"

/// <summary>
/// Implements a graph on which the visualizations are rendered.
/// </summary>
class graph_t : public element_t
{
public:
    graph_t(state_t & state, render_context_t & renderContext) : element_t(state), _Analysis(state), _RenderContext(renderContext) { }

    virtual ~graph_t() = default;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final { };
    void Reset() noexcept override final;

    void Configure(graph_options_t * graphOptions, bool isFirst, bool isLast) noexcept;
    void Process(const audio_chunk & chunk) noexcept;
    void Render(artwork_t & artwork) noexcept;

    void Initialize() noexcept;

    void InitToolInfo(HWND hParent, TTTOOLINFOW & ti) const noexcept;

    /// <summary>
    /// Returns true if the specified point lies within our client rectangle.
    /// </summary>
    bool ContainsPoint(const CPoint & pt) const noexcept
    {
        if ((FLOAT) pt.x < _Rect.left)
            return false;

        if ((FLOAT) pt.x > _Rect.right)
            return false;

        if ((FLOAT) pt.y < _Rect.top)
            return false;

        if ((FLOAT) pt.y > _Rect.bottom)
            return false;

        return true;
    }

    bool GetToolTipText(FLOAT x, FLOAT y, std::wstring & toolTip, size_t & index) const noexcept;

    void OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    void RenderBackground(artwork_t & artwork) noexcept;
    void RenderDescription() noexcept;

public:
    analysis_t _Analysis;

private:
    render_context_t & _RenderContext;

    std::wstring _Description;
    std::unique_ptr<visualization_t> _Visualization;

//  style_manager_t _StyleManager;                  // Styles only used by this graph in case of overlapping graphs.

    style_t _BackgroundStyle;
    style_t _DescriptionTextStyle;
    style_t _DescriptionBackgroundStyle;

#ifdef _DEBUG
    ComPtr<ID2D1SolidColorBrush> _DebugBrush;
#endif
};

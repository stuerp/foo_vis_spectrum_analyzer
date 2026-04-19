
/** $VER: Graph.h (2026.03.17) P. Stuer - Implements a graph on which the visualizations are rendered. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Artwork.h"

#include "Element.h"

/// <summary>
/// Implements a graph on which the visualizations are rendered.
/// </summary>
class graph_t : public element_t
{
public:
    graph_t();

    virtual ~graph_t() noexcept;

    // element_t
    void Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final { };
    void Reset() noexcept override final;
    void Release() noexcept override final;

    void Process(const audio_chunk & chunk) noexcept;
    void Render(ID2D1DeviceContext * deviceContext, artwork_t & artwork) noexcept;

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

    void OnConfigurationChange(ConfigurationChanges configurationChanges) noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

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

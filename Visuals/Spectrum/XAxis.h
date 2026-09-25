
/** $VER: XAxis.h (2026.09.25) P. Stuer - Implements the X axis of a graph. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Visualization.h"
#include "State.h"
#include "FrequencyBand.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the X axis of a graph.
/// </summary>
#pragma warning(disable: 4820)
class x_axis_t : public visualization_t
{
public:
    x_axis_t() = default;

    x_axis_t(const x_axis_t &) = delete;
    x_axis_t & operator=(const x_axis_t &) = delete;
    x_axis_t(x_axis_t &&) = delete;
    x_axis_t & operator=(x_axis_t &&) = delete;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext, IDXGISwapChain1 * swapChain) noexcept override final;
    void Reset() noexcept override final { }

    // visualization_t
    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext, style_manager_t & styleManager) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    FLOAT GetTextHeight() const noexcept
    {
        return _TextStyle._Height;
    }

    void Resize(bool force = false) noexcept;

private:
    size_t _BandCount { 0 };
    double _LoFrequency { 0. };
    double _HiFrequency { 0. };

    struct label_t
    {
        std::wstring Text;
        double Frequency { 0. };
        bool IsDimmed { false };
        bool IsHidden { false };

        D2D1_POINT_2F PointT { };
        D2D1_POINT_2F PointB { };

        D2D1_RECT_F RectT { };
        D2D1_RECT_F RectB { };
    };

    std::vector<label_t> _Labels;

    style_t _LineStyle;
    style_t _TextStyle;
};

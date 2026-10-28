
/** $VER: YAxis.h (2026.10.28) P. Stuer - Implements the Y axis of a graph. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Visualization.h"
#include "SpectrumProcessor.h"
#include "State.h"

#include <vector>
#include <string>

namespace Spectrum
{

/// <summary>
/// Implements the Y axis of a graph.
/// </summary>
class y_axis_t final : public visualization_t
{
public:
    y_axis_t(state_t & state) : visualization_t(state) { }

    y_axis_t(const y_axis_t &) = delete;
    y_axis_t(y_axis_t &&) = delete;

    y_axis_t & operator=(const y_axis_t &) = delete;
    y_axis_t & operator=(y_axis_t &&) = delete;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final { }

    // visualization_t
    void Configure(graph_options_t * options, audio_processor_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext, style_manager_t & styleManager) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

    FLOAT GetTextWidth() const noexcept
    {
        return _TextStyle._Width;
    }

private:
    HRESULT CreateSizeDependentResources(ID2D1DeviceContext * deviceContext, style_manager_t & styleManager) noexcept;
    void DeleteSizeDependentResources() noexcept;

    HRESULT CreateColorDependentResources(ID2D1DeviceContext * deviceContext, style_manager_t & styleManager) noexcept;
    void DeleteColorDependentResources() noexcept;

    void Initialize() noexcept;

private:
    processor_t * _Processor;
    bool _FlipVertically;

    struct label_t
    {
        std::wstring Text;
        double Amplitude { 0. };
        bool IsHidden { false };

        D2D1_POINT_2F PointL { };
        D2D1_POINT_2F PointR { };

        D2D1_RECT_F RectL { };
        D2D1_RECT_F RectR { };
    };

    std::vector<label_t> _Labels;

    style_t _LineStyle;
    style_t _TextStyle;
};

}

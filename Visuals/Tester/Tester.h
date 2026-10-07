
/** $VER: Tester.h (2026.10.05) P. Stuer - Implements a minimal visualization for testing purposes. **/

#pragma once

#include <pch.h>

#include "Visualization.h"

class tester_t final : public visualization_t
{
public:
    tester_t();

    tester_t(const tester_t &) = delete;
    tester_t & operator=(const tester_t &) = delete;
    tester_t(tester_t &&) = delete;
    tester_t & operator=(tester_t &&) = delete;

    virtual ~tester_t();

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;
    void Release() noexcept override final;

    void OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept override final;

    // visualization_t
    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;
    void Resize() noexcept;

private:
    HRESULT CreateDeviceIndependentResources() noexcept;
    void DeleteDeviceIndependentResources() noexcept;

    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

private:
    ComPtr<ID2D1SolidColorBrush> _Brush;
};

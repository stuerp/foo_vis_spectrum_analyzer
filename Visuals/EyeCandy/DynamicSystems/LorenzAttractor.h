
/** $VER: LorenzAttractor.h (2026.10.10) P. Stuer - Implements an effect for the Eye Candy visualizer. **/

#pragma once

#include "Effect.h"

#include <comdef.h> // For _com_error

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class lorenz_attractor_t final : public effect_t
{
public:
    lorenz_attractor_t() = default;

    lorenz_attractor_t(const lorenz_attractor_t &) = delete;
    lorenz_attractor_t & operator=(const lorenz_attractor_t &) = delete;
    lorenz_attractor_t(lorenz_attractor_t &&) = delete;
    lorenz_attractor_t & operator=(lorenz_attractor_t &&) = delete;

    virtual ~lorenz_attractor_t() = default;

    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;

private:
    HRESULT CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept;
    void DeleteDeviceSpecificResources() noexcept;

private:
    ComPtr<ID2D1SolidColorBrush> _Brush;

private:
    double _LeftRotationCount;
    double _RightRotationCount;

    uint64_t _MaxColorIndex;

    double _LorenzX = 0.1;
    double _LorenzY = 0.0;
    double _LorenzZ = 0.0;

    double _SmoothedLeftLevel  = 0.0;
    double _SmoothedRightLevel = 0.0;

    double _LeftRotationAngle  = 0.0;
    double _RightRotationAngle = 0.0;

    std::chrono::steady_clock::time_point _PreviousRenderTime = { };

    double _SmoothedLeftRotationSpeed  = 0.0;
    double _SmoothedRightRotationSpeed = 0.0;

    bool _IsRenderTimeInitialized = false;

    bool _IsWarmedUp = false;
};

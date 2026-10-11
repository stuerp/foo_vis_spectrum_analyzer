
/** $VER: EyeCandy.h (2026.10.10) P. Stuer - Implements a visualizer focusing on eye candy. **/

#pragma once

#include <pch.h>

#include "Visualization.h"

#include "Effect.h"

class eye_candy_t final : public visualization_t
{
public:
    eye_candy_t() = default;

    eye_candy_t(const eye_candy_t &) = delete;
    eye_candy_t & operator=(const eye_candy_t &) = delete;
    eye_candy_t(eye_candy_t &&) = delete;
    eye_candy_t & operator=(eye_candy_t &&) = delete;

    virtual ~eye_candy_t() = default;

    // element_t
    void Move(const D2D1_RECT_F & rect) noexcept override final;
    void Render(ID2D1DeviceContext * deviceContext) noexcept override final;
    void Reset() noexcept override final;

    void OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept override final;

    // visualization_t
    void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept override final;
    void Initialize() noexcept;

private:
    ComPtr<ID2D1SolidColorBrush> _Brush;

private:
    std::unique_ptr<effect_t> _Effect;
};

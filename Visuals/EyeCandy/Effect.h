
/** $VER: Effect.h (2026.10.10) P. Stuer - Base class for the Eye Candy effects. **/

#pragma once

#include <d3d11_2.h>
#include <d2d1_3.h>

#include "State.h"
#include "Graphoptions.h"
#include "Analysis.h"

class effect_t
{
public:
    effect_t() = default;

    effect_t(const effect_t &) = delete;
    effect_t & operator=(const effect_t &) = delete;
    effect_t(effect_t &&) = delete;
    effect_t & operator=(effect_t &&) = delete;

    virtual ~effect_t() = default;

    virtual void Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice = nullptr, ID3D11DeviceContext * d3dDeviceContext = nullptr) noexcept = 0;

    virtual void Move(const D2D1_RECT_F & rect) noexcept
    {
        _Rect     = rect;
        _Size     = { std::abs(rect.right - rect.left), std::abs(rect.bottom - rect.top) };
        _Side     = std::min(_Size.width, _Size.height);
        _HalfSide = _Side / 2.f;
    }

    virtual void Render(ID2D1DeviceContext * deviceContext) noexcept = 0;
    virtual void Reset() noexcept = 0;

protected:
    state_t * _State { nullptr };
    graph_options_t * _GraphOptions { nullptr };
    analysis_t * _Analysis { nullptr };

    bool _IsFirst { false };
    bool _IsLast { false };

    D2D1_RECT_F _Rect { };
    D2D1_SIZE_F _Size { };

    FLOAT _HalfSide { 0.f };
    FLOAT _Side { 0.f };
};

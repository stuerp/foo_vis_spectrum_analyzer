
/** $VER: EyeCandy.cpp (2026.10.10) P. Stuer - Implements a visualizer focusing on eye candy. **/

#include <pch.h>

#include "EyeCandy.h"

#include "DynamicSystems/LorenzAttractor.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void eye_candy_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    Reset();

    _Effect = make_unique<lorenz_attractor_t>();

    _Effect->Configure(state, graphOptions, analysis, isFirst, isLast, d3dDevice, d3dDeviceContext);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void eye_candy_t::Move(const D2D1_RECT_F & rect) noexcept
{
    _Effect->Move(rect);
}

/// <summary>
/// Renders this instance.
/// </summary>
void eye_candy_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    _Effect->Render(deviceContext);
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void eye_candy_t::Initialize() noexcept
{
}

/// <summary>
/// Resets this instance.
/// </summary>
void eye_candy_t::Reset() noexcept
{
}

/// <summary>
/// Handles a configuration change.
/// </summary>
void eye_candy_t::OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept
{
    if (!IsSet(configurationChanges, ConfigurationChanges::Layout))
        return;

    _State->_SizeHasChanged = true;
}

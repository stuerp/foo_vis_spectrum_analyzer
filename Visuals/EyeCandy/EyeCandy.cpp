
/** $VER: EyeCandy.cpp (2026.10.23) P. Stuer - Implements a visualizer focusing on eye candy. **/

#include <pch.h>

#include "EyeCandy.h"

/// <summary>
/// Initializes this instance.
/// </summary>
void eye_candy_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    Reset();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void eye_candy_t::Move(const D2D1_RECT_F & rect) noexcept
{
    _Effect->Resize((rect.right - rect.left), (rect.bottom - rect.top));
}

/// <summary>
/// Renders this instance.
/// </summary>
void eye_candy_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    if ((_EffectId != _State->_EffectId) || (_Effect == nullptr))
    {
        _Effect = EyeCandy::effect_t::Create(_State->_EffectId);

        _EffectId = _State->_EffectId;
    }

    _Effect->SetQuality(_State->_EffectQuality);
    _Effect->SetSpeed(_State->_EffectSpeed);
    _Effect->SetBrightness(_State->_EffectBrightness);
    _Effect->SetDensity(_State->_EffectDensity);

    double DeltaTime = _State->_PlaybackTime - _PlaybackTime;

    _Effect->Render(deviceContext, DeltaTime, _Analysis->_Chunk);

    _PlaybackTime = _State->_PlaybackTime;
}

/// <summary>
/// Resets this instance.
/// </summary>
void eye_candy_t::Reset() noexcept
{
    if ((_EffectId != _State->_EffectId) || (_Effect == nullptr))
    {
        _Effect = EyeCandy::effect_t::Create(_State->_EffectId);

        _EffectId = _State->_EffectId;
    }

    _PlaybackTime = _State->_PlaybackTime;
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

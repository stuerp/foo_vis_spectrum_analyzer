
/** $VER: Effect.h (2026.10.23) P. Stuer - Base class for the Eye Candy effects. **/

#include <pch.h>

#include "Effect.h"

#include "Effects/DynamicSystems/AizawaAttractor.h"
#include "Effects/DynamicSystems/HalvorsenAttractor.h"
#include "Effects/DynamicSystems/LorenzAttractor.h"
#include "Effects/DynamicSystems/RosslerAttractor.h"
#include "Effects/DynamicSystems/ThomasAttractor.h"

#include "Effects/DynamicSystems/CliffordAttractor.h"
#include "Effects/DynamicSystems/DeJongAttractor.h"

#include "Effects/DynamicSystems/DoublePendulum.h"

#include "Effects/Fields/Plasma.h"
#include "Effects/Fields/FluidInk.h"
#include "Effects/Fields/Aurora.h"
#include "Effects/Fields/Metaballs.h"

#include "Effects/ParticleSystems/Fireflies.h"
#include "Effects/ParticleSystems/Fireworks.h"
#include "Effects/ParticleSystems/ParticleVortex.h"
#include "Effects/ParticleSystems/Starfield.h"
#include "Effects/ParticleSystems/AudioGalaxy.h"
#include "Effects/ParticleSystems/Swarm.h"

#include "Effects/Retro/NeonTunnel.h"
#include "Effects/Retro/VectorGrid.h"

#include "Effects/Trails/Waveform.h"
#include "Effects/Trails/LissajousTrails.h"
#include "Effects/Trails/RibbonFlow.h"
#include "Effects/Trails/Spirograph.h"

namespace EyeCandy
{

std::unique_ptr<effect_t> effect_t::Create(Effect effect) noexcept
{
    switch (effect)
    {
        default:
        case Effect::Waveform:
            return std::make_unique<Waveform::effect_t>();

        // Dynamic Systems
        case Effect::LorenzAttractor:
            return std::make_unique<LorenzAttractor::effect_t>();

        case Effect::RosslerAttractor:
            return std::make_unique<RosslerAttractor::effect_t>();

        case Effect::AizawaAttractor:
            return std::make_unique<AizawaAttractor::effect_t>();

        case Effect::HalvorsenAttractor:
            return std::make_unique<HalvorsenAttractor::effect_t>();

        case Effect::ThomasAttractor:
            return std::make_unique<ThomasAttractor::effect_t>();

        case Effect::CliffordAttractor:
            return std::make_unique<CliffordAttractor::effect_t>();

        case Effect::DeJongAttractor:
            return std::make_unique<DeJongAttractor::effect_t>();

        case Effect::DoublePendulum:
            return std::make_unique<DoublePendulum::effect_t>();

        // Fields
        case Effect::Plasma:
            return std::make_unique<Plasma::effect_t>();

        case Effect::FluidInk:
            return std::make_unique<FluidInk::effect_t>();

        case Effect::Aurora:
            return std::make_unique<Aurora::effect_t>();

        case Effect::Metaballs:
            return std::make_unique<Metaballs::effect_t>();

        // Trails
        case Effect::LissajousTrails:
            return std::make_unique<LissajousTrails::effect_t>();

        case Effect::RibbonFlow:
            return std::make_unique<RibbonFlow::effect_t>();

        case Effect::Spirograph:
            return std::make_unique<Spirograph::effect_t>();

        // Particle Systems
        case Effect::Fireflies:
            return std::make_unique<Fireflies::effect_t>();

        case Effect::Fireworks:
            return std::make_unique<Fireworks::effect_t>();

        case Effect::ParticleVortex:
            return std::make_unique<ParticleVortex::effect_t>();

        case Effect::Starfield:
            return std::make_unique<Starfield::effect_t>();

        case Effect::AudioGalaxy:
            return std::make_unique<AudioGalaxy::effect_t>();

        case Effect::Swarm:
            return std::make_unique<Swarm::effect_t>();

        // Retro
        case Effect::NeonTunnel:
            return std::make_unique<NeonTunnel::effect_t>();

        case Effect::VectorGrid:
            return std::make_unique<VectorGrid::effect_t>();
    }
}

/// <summary>
/// 
/// </summary>
HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget)
{
    if (renderTarget && (_RenderTarget == renderTarget))
        return S_OK;

    if (_ScanlineBrush && _FrameBrush && _BackgroundBrush)
        return S_OK;

    renderTarget->GetFactory(_D2DFactory.GetAddressOf());

    if (!_D2DFactory)
        return E_FAIL;

    HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0.05f), _ScanlineBrush.ReleaseAndGetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = renderTarget->CreateSolidColorBrush(GetThemeColor(ThemeRole::Ring, 0, 0.8f, 0.8f, 0.2f), _FrameBrush.ReleaseAndGetAddressOf());

    if (FAILED(hr))
        return hr;

    const D2D1_GRADIENT_STOP background[] =
    {
        { 0.00f, GetThemeColor(ThemeRole::BackgroundA, -0.15f, 0.88f, 0.140f, 1) },
        { 0.36f, GetThemeColor(ThemeRole::BackgroundA,  0.15f, 0.82f, 0.070f, 1) },
        { 0.72f, GetThemeColor(ThemeRole::BackgroundB, -0.10f, 0.74f, 0.035f, 1) },
        { 1.00f, GetThemeColor(ThemeRole::BackgroundB,  0.18f, 0.65f, 0.008f, 1) }
    };

    hr = renderTarget->CreateGradientStopCollection(background, _countof(background), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, _BackgroundStops.ReleaseAndGetAddressOf());

    if (FAILED(hr))
        return hr;

    hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(_Width, _Height)), _BackgroundStops.Get(), _BackgroundBrush.ReleaseAndGetAddressOf());

    if (FAILED(hr))
        return hr;

    _RenderTarget = renderTarget;

    return S_OK;
}

/// <summary>
/// 
/// </summary>
void effect_t::DeleteDeviceDependentResources() noexcept
{
    _ScanlineBrush.Reset();
    _FrameBrush.Reset();

    _BackgroundStops.Reset();
    _BackgroundBrush.Reset();

    _D2DFactory.Reset();

    _RenderTarget = nullptr;
}

/// <summary>
/// Renders the background with a subtle shine in the left top corner.
/// </summary>
void effect_t::RenderBackground(ID2D1RenderTarget * renderTarget) noexcept
{
    renderTarget->Clear(D2D1::ColorF(0.001f, 0.001f, 0.006f, 1.f));
}

/// <summary>
/// Renders a frame around the viewport.
/// </summary>
void effect_t::RenderFrame(ID2D1RenderTarget * renderTarget, const D2D1_COLOR_F & color) noexcept
{
    _FrameBrush->SetColor(color);

    renderTarget->DrawRectangle(D2D1::RectF(0.5f, 0.5f, _Width - 0.5f, _Height - 0.5f), _FrameBrush.Get(), 1.f);
}

/// <summary>
/// Renders scanlines over the final image.
/// </summary>
void effect_t::RenderScanlines(ID2D1RenderTarget * renderTarget) noexcept
{
    const float Opacity = 0.04f + (float) (_SmoothedBass * 0.015);

    _ScanlineBrush->SetColor(D2D1::ColorF(0.f, 0.f, 0.f, Opacity));

    const float Spacing = [&]() noexcept
    {
        switch (_Quality)
        {
            case EyeCandy::Quality::Low:
                return 6.0f;

            case EyeCandy::Quality::High:
                return 3.0f;

            case EyeCandy::Quality::Medium:
            default:
                return 4.0f;
        }
    }();

    const float Offset = Fraction((float) (_AnimationTime * 5.)) * Spacing;

    for (float y = Offset; y < _Height; y += Spacing)
        renderTarget->DrawLine(D2D1::Point2F(0.0f, y), D2D1::Point2F(_Width, y), _ScanlineBrush.Get(), 1.0f);
}

/// <summary>
/// 
/// </summary>
D2D1_COLOR_F effect_t::LerpColor(const D2D1_COLOR_F & from, const D2D1_COLOR_F & to, float amount) noexcept
{
    amount = Clamp(amount);

    return D2D1::ColorF(Lerp(from.r, to.r, amount), Lerp(from.g, to.g, amount), Lerp(from.b, to.b, amount), Lerp(from.a, to.a, amount));
}

/// <summary>
/// 
/// </summary>
D2D1_COLOR_F effect_t::GetThemeColor(const color_theme_t & theme, float hue, float offset, float saturation, float value, float alpha) const noexcept
{
    value = std::clamp(value * theme.Brightness, 0.0f, 1.0f);

    if (theme.Monochrome) return D2D1::ColorF(value, value, value, alpha);

    return HSVToColor(hue + offset * theme.HueRange, saturation * theme.Saturation, value, alpha);
}

/// <summary>
/// 
/// </summary>
D2D1_COLOR_F effect_t::GetThemeColor(ThemeRole role, float offset, float saturation, float value, float alpha) const noexcept
{
    const auto & a = GetTheme(_ThemeFrom);
    const auto & b = GetTheme(_ThemeTo);

    float ah = a.RingHue;
    float bh = b.RingHue;

    switch (role)
    {
        case ThemeRole::BackgroundA: ah = a.BackgroundHueA; bh = b.BackgroundHueA; break;
        case ThemeRole::BackgroundB: ah = a.BackgroundHueB; bh = b.BackgroundHueB; break;
        case ThemeRole::Rail:        ah = a.RailHue;        bh = b.RailHue; break;
        case ThemeRole::Core:        ah = a.CoreHue;        bh = b.CoreHue; break;
        case ThemeRole::Flash:       ah = a.FlashHue;       bh = b.FlashHue; break;

        case ThemeRole::Ring:
        default:
            break;
    }

    return LerpColor(GetThemeColor(a, ah, offset, saturation, value, alpha), GetThemeColor(b, bh, offset, saturation, value, alpha), (float) _ThemeBlend);
}

/// <summary>
/// 
/// </summary>
const color_theme_t& effect_t::GetTheme(ColorTheme theme) noexcept
{
    static const color_theme_t Themes[] =
    {
        { L"Synthwave",  0.86f, 0.58f, 0.56f, 0.88f, 0.53f, 0.84f, 0.30f, 0.92f, 1.00f, false },
        { L"Cyberpunk",  0.55f, 0.92f, 0.50f, 0.91f, 0.47f, 0.14f, 0.46f, 0.96f, 1.00f, false },
        { L"Tron",       0.56f, 0.63f, 0.52f, 0.58f, 0.54f, 0.51f, 0.10f, 0.88f, 1.00f, false },
        { L"Toxic",      0.28f, 0.48f, 0.27f, 0.42f, 0.31f, 0.18f, 0.22f, 0.96f, 0.96f, false },
        { L"Inferno",    0.98f, 0.07f, 0.02f, 0.09f, 0.04f, 0.13f, 0.14f, 0.94f, 1.00f, false },
        { L"Monochrome", 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f, true }
    };

    const size_t Index = (size_t) std::clamp(theme, (ColorTheme) 0, ColorTheme::Count);

    return Themes[Index];
}

/// <summary>
/// 
/// </summary>
const std::wstring effect_t::GetThemeName(ColorTheme theme) noexcept
{
    return GetTheme(theme).Name;
}

/// <summary>
/// 
/// </summary>
void effect_t::SetColorTheme(ColorTheme theme) noexcept
{
    BeginThemeTransition((size_t)theme < (size_t)ColorTheme::Count ? theme : ColorTheme::Synthwave);
}

/// <summary>
/// 
/// </summary>
void effect_t::NextColorTheme() noexcept
{
    BeginThemeTransition((ColorTheme)(((size_t)_ThemeTo + 1) % (size_t)ColorTheme::Count));
}

/// <summary>
/// 
/// </summary>
void effect_t::PreviousColorTheme() noexcept
{
    const size_t count = (size_t)ColorTheme::Count;
    BeginThemeTransition((ColorTheme)(((size_t)_ThemeTo + count - 1) % count));
}

/// <summary>
/// 
/// </summary>
void effect_t::BeginThemeTransition(ColorTheme theme) noexcept
{
    _ThemeFrom = _ThemeTransitionActive && (_ThemeBlend >= 0.5) ? _ThemeTo : _ThemeFrom;

    if (!_ThemeTransitionActive)
        _ThemeFrom = _ThemeTo;

    _ThemeTo    = theme;
    _ColorTheme = theme;

    _ThemeBlend             = (_ThemeFrom == _ThemeTo) ? 1. : 0.;
    _ThemeTransitionElapsed = (_ThemeFrom == _ThemeTo) ? ThemeTransitionDuration : 0.;
    _ThemeTransitionActive  = (_ThemeFrom != _ThemeTo);
    _ThemeResourceElapsed   = ThemeResourceUpdateInterval;
    _ThemeResourcesDirty    = true;
}

/// <summary>
/// 
/// </summary>
void effect_t::UpdateThemeTransition(double deltaTime) noexcept
{
    _ThemeElapsed += deltaTime;

    if (!_ThemeTransitionActive && _ThemeElapsed >= ThemeInterval)
    {
        _ThemeElapsed = std::fmod(_ThemeElapsed, ThemeInterval);

        NextColorTheme();
    }

    if (!_ThemeTransitionActive)
        return;

    _ThemeTransitionElapsed += deltaTime;
    _ThemeResourceElapsed   += deltaTime;

    const double Linear = (float) Clamp(_ThemeTransitionElapsed / ThemeTransitionDuration);

    _ThemeBlend = Linear * Linear * (3. - 2. * Linear);

    if (_ThemeResourceElapsed >= ThemeResourceUpdateInterval)
    {
        _ThemeResourceElapsed = std::fmod(_ThemeResourceElapsed, ThemeResourceUpdateInterval);
        _ThemeResourcesDirty = true;
    }

    if (Linear >= 1.)
    {
        _ThemeFrom             = _ThemeTo;
        _ThemeBlend            = 1.;
        _ThemeTransitionActive = false;
        _ThemeResourcesDirty   = true;
    }
}

}

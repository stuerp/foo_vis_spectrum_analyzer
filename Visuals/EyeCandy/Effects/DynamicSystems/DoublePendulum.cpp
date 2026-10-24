
/** $VER: DoublePendulum.cpp (2026.10.22) Implements a double pendulum visual effect. https://en.wikipedia.org/wiki/Double_pendulum */

#include <pch.h>

#include "DoublePendulum.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace DoublePendulum
{
    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Double Pendulum, {} theme, {} quality, {} speed, {} brightness",
            CurveThemes[_CurveThemeTo].Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        _Points.clear();
        _SplinePoints.clear();
        _PendulumState = {};

        __super::Reset();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = std::clamp(deltaTime, 0., 0.1);

        AnalyzeAudio(chunk);
        Update(deltaTime);

        UpdateScene(deltaTime);
        UpdatePendulum();

        {
            _BackBufferRenderTarget->BeginDraw();

            ID2D1RenderTarget * RenderTarget = _BackBufferRenderTarget.Get();

            Fade(RenderTarget, deltaTime);

            RenderCurve(RenderTarget);
            RenderPendulum(RenderTarget);

            hr = _BackBufferRenderTarget->EndDraw();

            if (FAILED(hr))
            {
                DeleteDeviceDependentResources();

                return;
            }
        }

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);

        RenderAxes(renderTarget, 0.5);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case EyeCandy::Speed::Slow: return 0.55;
                case EyeCandy::Speed::Fast: return 1.75;
                case EyeCandy::Speed::Normal:
                default: return 1.;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        _SmoothedPeak        = ExponentialSmoothing(_SmoothedPeak,        _Peak,        deltaTime, (_Peak > _SmoothedPeak) ? 22. : 5.);
        _SmoothedRMS         = ExponentialSmoothing(_SmoothedRMS,         _RMS,         deltaTime, (_RMS  > _SmoothedRMS)  ? 16. : 4.);

        _SmoothedWidth       = ExponentialSmoothing(_SmoothedWidth,       _StereoWidth, deltaTime, 5.5);
        _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _Correlation, deltaTime, 4.);

        UpdateCurveTheme(deltaTime);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateScene(double deltaTime) noexcept
    {
        const double Peak  = Clamp(_SmoothedPeak);
        const double RMS   = Clamp(_SmoothedRMS);
        const double Width = Clamp(_SmoothedWidth);
        const double Phase = Clamp(_PhaseEnergy);

        _AudioOrbit = ExponentialSmoothing(_AudioOrbit, Width * 0.24,                   deltaTime, 3.5);
        _AudioLift  = ExponentialSmoothing(_AudioLift, (Peak - RMS) * 0.18,             deltaTime, 5.);
        _AudioScale = ExponentialSmoothing(_AudioScale, 1. + RMS * 0.08 + Peak * 0.025, deltaTime, 7.);

        const double Time = _AnimationTime;

        World = {};
        World.Scale = { 1., 1., 1. };

        Object.Position = { 0., -0.15 + _AudioLift, 0. };
        Object.Rotation =
        {
            0.24 + std::sin(Time * 0.19) * 0.06 + Phase * 0.06,
            Time * (0.075 + RMS * 0.045) + std::sin(Time * 0.13) * 0.10,
            std::sin(Time * 0.23) * (0.035 + Width * 0.08)
        };
        Object.Scale = { _AudioScale, _AudioScale, _AudioScale };

        const double Orbit = (Time * 0.035) + (_AudioOrbit * std::sin(Time * 0.31));
        const double Radius = 6.8 - RMS * 0.35;

        Camera.Position =
        {
                  std::sin(Orbit)       * Radius,
            1.2 + std::sin(Time * 0.11) * 0.35,
                 -std::cos(Orbit)       * Radius
        };
        Camera.Target = { Object.Position.X, Object.Position.Y + 0.75, 0. };
        Camera.Up     = { 0., 1., 0. };

        Projection.FieldOfView = 48. - Peak * 2.;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdatePendulum() noexcept
    {
        const size_t PointCount = [&]() noexcept -> size_t
        {
            switch (_Quality)
            {
                case Quality::Low:
                    return 1'200;

                default:
                case Quality::Medium:
                    return 3'000;

                case Quality::High:
                    return 6'000;
            }
        }();

        _Field =
        {
            .PointCount = PointCount,
            .Gravity    = 9.81   + (_SmoothedPeak * 0.75),
            .Length1    = 1.,
            .Length2    = 0.92   + (_SmoothedWidth * 0.025),
            .Mass1      = 1.,
            .Mass2      = 1.     + (_PhaseEnergy * 0.08),
            .Damping    = 0.0015 + (_SmoothedRMS * 0.0015),
            .Step       = 0.0035
        };

        const size_t Advances = 2 + (size_t) std::clamp(_SmoothedPeak * 4., 0., 4.);

        for (size_t i = 0; i < Advances; ++i)
            _PendulumState = Integrate(_PendulumState, _Field);

        state_t Preview = _PendulumState;

        _Points.clear();
        _Points.reserve(PointCount);

        for (size_t i = 0; i < PointCount; ++i)
        {
            Preview = Integrate(Preview, _Field);

            const double X1    =      _Field.Length1 * std::sin(Preview.Theta1);
            const double Y1    =     -_Field.Length1 * std::cos(Preview.Theta1);
            const double X2    = X1 + _Field.Length2 * std::sin(Preview.Theta2);
            const double Y2    = Y1 - _Field.Length2 * std::cos(Preview.Theta2);
            const double Speed = std::sqrt(Preview.Omega1 * Preview.Omega1 + Preview.Omega2 * Preview.Omega2);

            _Points.push_back({ X2, Y2, std::clamp(Speed * 0.06, 0., 0.85) });
        }

        const double X1 =  _Field.Length1 * std::sin(_PendulumState.Theta1);
        const double Y1 = -_Field.Length1 * std::cos(_PendulumState.Theta1);

        _PivotWorld = { 0., 0., 0. };
        _Bob1World = { X1, Y1, 0. };
        _Bob2World =
        {
            X1 + _Field.Length2 * std::sin(_PendulumState.Theta2),
            Y1 - _Field.Length2 * std::cos(_PendulumState.Theta2),
            0.
        };

        GenerateSpline();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderPendulum(ID2D1RenderTarget * renderTarget) noexcept
    {
        const auto PreparedProjection = ThreeD::Prepare();

        if (!PreparedProjection.IsValid)
            return;

        const auto Pivot = Project(_PivotWorld, PreparedProjection);
        const auto Bob1  = Project(_Bob1World,  PreparedProjection);
        const auto Bob2  = Project(_Bob2World,  PreparedProjection);

        const auto ArmColor = GetCurveColor(_AnimationTime * 0.035);
        const auto BobColor = GetCurveColor(0.55 + _AnimationTime * 0.035);

        if (Pivot.IsProjectable && Bob1.IsProjectable)
            DrawGlowLine(renderTarget, Pivot.Position, Bob1.Position, ArmColor, 0.80f, 1.15f);

        if (Bob1.IsProjectable && Bob2.IsProjectable)
            DrawGlowLine(renderTarget, Bob1.Position, Bob2.Position, BobColor, 1.f, 1.25f);

        if (Pivot.IsProjectable)
            DrawAmbientGlow(renderTarget, Pivot.Position, ArmColor, 0.10f, 3.5f);

        if (Bob1.IsProjectable)
            DrawAmbientGlow(renderTarget, Bob1.Position, ArmColor, 0.12f, 5.5f);

        if (Bob2.IsProjectable)
            DrawAmbientGlow(renderTarget, Bob2.Position, BobColor, 0.12f, 7.f + (float) _SmoothedPeak * 4.f);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderCurve(ID2D1RenderTarget * renderTarget) noexcept
    {
        if ((_Width < 2.f) || (_Height < 2.f) || (_SplinePoints.size() < 2))
            return;

        const double Energy = std::clamp(0.32 + (_SmoothedRMS * 2.2) + (_SmoothedPeak * 0.35), 0.32, 1.);

        const auto PreparedProjection = ThreeD::Prepare();

        if (!PreparedProjection.IsValid)
            return;

        auto Previous = Project(_SplinePoints.front(), PreparedProjection);

        for (size_t i = 1; i < _SplinePoints.size(); ++i)
        {
            const auto Current = Project(_SplinePoints[i], PreparedProjection);

            if (Previous.IsProjectable && Current.IsProjectable)
            {
                const double Amount    = (double) i / (double) (_SplinePoints.size() - 1);
                const double Depth     = 1. - (Previous.LinearDepth + Current.LinearDepth) * .5;
                const double ViewDepth = (Previous.ViewDepth + Current.ViewDepth) * .5;

                const float Intensity = (float) (Energy * (0.52 + Depth * 0.48));
                const float Thickness = std::clamp(6.8f / (float) std::max(ViewDepth, Projection.NearPlane), 0.50f, 1.90f);

                DrawGlowLine(renderTarget, Previous.Position, Current.Position, GetCurveColor(Amount + _AnimationTime * 0.025), Intensity, Thickness);
            }

            Previous = Current;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawAmbientGlow(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F center, const D2D1_COLOR_F & color, float intensity, float radius) noexcept
    {
        intensity = std::clamp(intensity, 0.f, 0.12f);

        if (intensity <= 0.0001f)
            return;

        SetBrush(color, intensity);

        renderTarget->FillEllipse(D2D1::Ellipse(center, std::max(0.5f, radius), std::max(0.5f, radius)), _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::state_t effect_t::Derive(state_t state, const field_t & field) noexcept
    {
        const double Delta = state.Theta1 - state.Theta2;

        const double SinDelta = std::sin(Delta);
        const double CosDelta = std::cos(Delta);

        const double Common = 2. * field.Mass1 + field.Mass2 - field.Mass2 * std::cos(2. * Delta);

        const double Denominator1 = std::max(0.000001, field.Length1 * Common);
        const double Denominator2 = std::max(0.000001, field.Length2 * Common);

        const double Acceleration1 =
        (
            -field.Gravity * (2. * field.Mass1 + field.Mass2) * std::sin(state.Theta1)
            - field.Mass2 * field.Gravity * std::sin(state.Theta1 - 2. * state.Theta2)
            - 2. * SinDelta * field.Mass2 * (state.Omega2 * state.Omega2 * field.Length2 + state.Omega1 * state.Omega1 * field.Length1 * CosDelta)
        ) / Denominator1 - field.Damping * state.Omega1;

        const double Acceleration2 =
        (
            2. * SinDelta *
            (
                state.Omega1  * state.Omega1 * field.Length1 * (field.Mass1 + field.Mass2) +
                field.Gravity * (field.Mass1 + field.Mass2) * std::cos(state.Theta1) +
                state.Omega2  * state.Omega2 * field.Length2 * field.Mass2 * CosDelta
            )
        ) / Denominator2 - field.Damping * state.Omega2;

        return { state.Omega1, Acceleration1, state.Omega2, Acceleration2 };
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::state_t effect_t::Integrate(state_t state, const field_t & field) noexcept
    {
        const double Step = field.Step;

        const state_t k1 = Derive(state, field);
        const state_t k2 = Derive({ state.Theta1 + k1.Theta1 * Step * .5, state.Omega1 + k1.Omega1 * Step * .5, state.Theta2 + k1.Theta2 * Step * .5, state.Omega2 + k1.Omega2 * Step * .5 }, field);
        const state_t k3 = Derive({ state.Theta1 + k2.Theta1 * Step * .5, state.Omega1 + k2.Omega1 * Step * .5, state.Theta2 + k2.Theta2 * Step * .5, state.Omega2 + k2.Omega2 * Step * .5 }, field);
        const state_t k4 = Derive({ state.Theta1 + k3.Theta1 * Step, state.Omega1 + k3.Omega1 * Step, state.Theta2 + k3.Theta2 * Step, state.Omega2 + k3.Omega2 * Step }, field);

        return
        {
            state.Theta1 + Step * (k1.Theta1 + 2. * k2.Theta1 + 2. * k3.Theta1 + k4.Theta1) / 6.,
            state.Omega1 + Step * (k1.Omega1 + 2. * k2.Omega1 + 2. * k3.Omega1 + k4.Omega1) / 6.,
            state.Theta2 + Step * (k1.Theta2 + 2. * k2.Theta2 + 2. * k3.Theta2 + k4.Theta2) / 6.,
            state.Omega2 + Step * (k1.Omega2 + 2. * k2.Omega2 + 2. * k3.Omega2 + k4.Omega2) / 6.
        };
    }
}

/** $VER: VanDerPolOscillator.cpp (2026.11.04) Audio-reactive forced Van der Pol oscillator. */

#include <pch.h>

#include "VanDerPolOscillator.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace VanDerPolOscillator
{
    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Van der Pol Oscillator, {} theme, {} quality, {} speed, {} brightness",
            CurveThemes[_CurveThemeTo].Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        DynamicSystem::effect_t::Reset();

        _AttractorState   = { .15, 0., 0. };
        _AttractorSettled = false;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = Clamp(deltaTime);

        AnalyzeAudio(chunk);

        Update(deltaTime);
        UpdateScene(deltaTime);

        UpdateAttractor();

        {
            _BackBufferRenderTarget->BeginDraw();

            ID2D1RenderTarget * RenderTarget = _BackBufferRenderTarget.Get();

            Fade(RenderTarget, deltaTime);
            RenderCurve(RenderTarget);

            hr = _BackBufferRenderTarget->EndDraw();

            if (FAILED(hr))
            {
                DeleteDeviceDependentResources();

                return;
            }
        }

        {
            renderTarget->DrawBitmap(_BackBuffer.Get(), _ViewPort);

            RenderAxes(renderTarget, 5.);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime)
    {
        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case Speed::Slow:
                    return 0.55;

                default:
                case Speed::Normal:
                    return 1.0;

                case Speed::Fast:
                    return 1.75;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        {
            _SmoothedPeak        = ExponentialSmoothing(_SmoothedPeak,        _Peak,        deltaTime, (_Peak > _SmoothedPeak) ? 22. : 5.);
            _SmoothedRMS         = ExponentialSmoothing(_SmoothedRMS,         _RMS,         deltaTime, (_RMS  > _SmoothedRMS)  ? 16. : 4.);

            _SmoothedStereoWidth = ExponentialSmoothing(_SmoothedStereoWidth, _StereoWidth, deltaTime, 5.5);
            _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _Correlation, deltaTime, 4.0);
        }

        UpdateCurveTheme(deltaTime);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderCurve(ID2D1RenderTarget * renderTarget) noexcept
    {
        if ((_Width < 2.f) || (_Height < 2.f) || (_SplinePoints.size() < 2))
            return;

        const double Energy = std::clamp(0.32 + (_SmoothedRMS * 2.2) + (_SmoothedPeak * 0.35), 0.32, 1.);

        // Synchronize the viewport once and prepare all camera/projection constants once per frame.
        Projection.Width       = _Width;
        Projection.Height      = _Height;
        Projection.AspectRatio = (double) _Width / (double) _Height;
        Projection.NearPlane   = .035;
        Projection.FarPlane    = 120.;

        const prepared_projection_t PreparedProjection = EyeCandy::ThreeD::Prepare();

        if (!PreparedProjection.IsValid)
            return;

        const double SplinePointScale = 1. / (double) (_SplinePoints.size() - 1);

        point3d_t PreviousPoint = _SplinePoints.front();

        for (size_t i = 1; i < _SplinePoints.size(); ++i)
        {
            point3d_t From = PreviousPoint;
            point3d_t To   = _SplinePoints[i];

            // Clip crossing segments rather than dropping both projected endpoints.
            if (EyeCandy::ThreeD::ClipLineToNearPlane(From, To, PreparedProjection))
            {
                const projected_point_t Previous = EyeCandy::ThreeD::Project(From, PreparedProjection);
                const projected_point_t Current  = EyeCandy::ThreeD::Project(To,   PreparedProjection);

                if (Previous.IsProjectable && Current.IsProjectable)
                {
                    const double Amount = (double) i * SplinePointScale; // [0, 1]

                    const auto Color = GetCurveColor(Amount + (_AnimationTime * 0.025));

                    const double Depth     = Clamp(1. - (Previous.LinearDepth + Current.LinearDepth) * .5);
                    const double ViewDepth =            (Previous.ViewDepth   + Current.ViewDepth)   * .5;

                    const float Intensity = (float) (Energy * (.52 + Depth * .48));
                    const float Thickness = std::clamp(28.f / (float) std::max(ViewDepth, Projection.NearPlane), .55f, 2.20f);

                    DrawGlowLine(renderTarget, Previous.Position, Current.Position, Color, Intensity, Thickness);
                }
            }

            PreviousPoint = _SplinePoints[i];
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateScene(double deltaTime) noexcept
    {
        const double Peak  = Clamp(_SmoothedPeak);
        const double RMS   = Clamp(_SmoothedRMS);
        const double Width = Clamp(_SmoothedStereoWidth);
        const double Phase = Clamp(_PhaseEnergy);

        _AudioOrbit = ExponentialSmoothing(_AudioOrbit, Width * .48,                      deltaTime, 3.6);
        _AudioLift  = ExponentialSmoothing(_AudioLift, (Peak - RMS) * 1.3,                deltaTime, 5.0);
        _AudioScale = ExponentialSmoothing(_AudioScale, 1. + (RMS * .11) + (Peak * .035), deltaTime, 7.0);

        const double Time = _AnimationTime;

        World = {};
        World.Scale = { 1., 1., 1. };

        Object.Position = { 0., _AudioLift, 0. };
        Object.Rotation =
        {
            .24                           + std::sin(Time * .16) * .11 + (Phase * .055),
            Time * (.065 + RMS * .045)    + std::sin(Time * .10) * .16,
                                            std::sin(Time * .22) * (.045 + (Width * 0.11))
        };
        Object.Scale = { _AudioScale, _AudioScale, _AudioScale };

        const double Orbit  = (Time * .036) + (_AudioOrbit * std::sin(Time * 0.25));
        const double Radius = 26. - (RMS * 1.6);

        Camera.Position =
        {
                   std::sin(Orbit) * Radius,
            2.6 + (std::sin(Time * .09) * 1.5) + (_AudioLift * .3),
                  -std::cos(Orbit) * Radius
        };
        Camera.Target = { 0., 0., 0. };
        Camera.Up     = { 0., 1., 0. };

        Projection.FieldOfView = 49. - (Peak * 2.8);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateAttractor() noexcept
    {
        const size_t PointCount = [&]() noexcept -> size_t
        {
            switch (_Quality)
            {
                case Quality::Low:
                    return 2500;

                default:
                case Quality::Medium:
                    return 5000;

                case Quality::High:
                    return 10000;
            }
        }();

        // Forced (or driven) Van der Pol oscillator: Audio modulates the relaxation strength and forcing while remaining bounded.
        //  x'     = y
        //  y'     = mu * (1 - x^2) * y - x + A * cos(theta) 
        //  theta' = omega
        const double Mu      = std::clamp(1.75 + _SmoothedRMS         * 1.65 + _SmoothedPeak * .320, 1.55, 3.55);
        const double Forcing = std::clamp(0.12 + _SmoothedPeak        * 0.22 + _PhaseEnergy  * .065,  .08,  .42); // Forcing amplitude
        const double Omega   = std::clamp(0.88 + _SmoothedStereoWidth * 0.18 + _SmoothedRMS  * .040,  .84, 1.12); // Forcing frequency

        constexpr double IntegrationStep = 0.006;

        // Burn in the initial state once or after a reset. Subsequent frames continue the trajectory while audio smoothly varies the oscillator parameters.
        if (!_AttractorSettled)
        {
            constexpr double SettlingTime = 25.0; // Simulated time units

            const size_t SettlingSteps = (size_t) std::ceil(SettlingTime / IntegrationStep); // ~4167 iterations with integration step = 0.006

            // Settle onto the driven limit cycle before emitting the visible trajectory.
            for (size_t i = 0; i < SettlingSteps; ++i)
                _AttractorState = Integrate(_AttractorState, IntegrationStep, Mu, Forcing, Omega);

            _AttractorSettled = true;
        }

        _Points.resize(PointCount);

        for (auto & Point : _Points)
        {
            _AttractorState = Integrate(_AttractorState, IntegrationStep, Mu, Forcing, Omega);

            Point = _AttractorState;
        }

        const auto Invalid = std::find_if(_Points.begin(), _Points.end(), [](const point3d_t & p)
        {
            return !std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z);
        });

        _Points.erase(Invalid, _Points.end());

        // Embed the forcing phase as depth. This preserves the x/y phase portrait while turning successive relaxation cycles into a gently breathing 3D ribbon.
        for (auto & Point : _Points)
        {
            const double Theta = Point.Z;

            Point.X *= 4.7;
            Point.Y *= 2.15;
            Point.Z  = std::sin(Theta)      * (3.00 + _SmoothedStereoWidth * 1.15)
                     + std::sin(Theta * 2.) * (0.35 + _SmoothedPeak        * 0.25);
        }

        GenerateSpline();
    }

    /// <summary>
    /// Integrates the equation using fourth-order Runge-Kutta. s = Integration step, a = Mu, b = forcing amplitude, c = omega.
    /// </summary>
    effect_t::point3d_t effect_t::Integrate(point3d_t p, double s, double a, double b, double c) noexcept
    {
        const auto k1 = Derive(p, a, b, c);
        const auto k2 = Derive({ p.X + k1.X * s * .5, p.Y + k1.Y * s * .5, p.Z + k1.Z * s * .5 }, a, b, c);
        const auto k3 = Derive({ p.X + k2.X * s * .5, p.Y + k2.Y * s * .5, p.Z + k2.Z * s * .5 }, a, b, c);
        const auto k4 = Derive({ p.X + k3.X * s,      p.Y + k3.Y * s,      p.Z + k3.Z * s },      a, b, c);

        point3d_t Result =
        {
            p.X + s * (k1.X + 2. * k2.X + 2. * k3.X + k4.X) / 6.,
            p.Y + s * (k1.Y + 2. * k2.Y + 2. * k3.Y + k4.Y) / 6.,
            p.Z + s * (k1.Z + 2. * k2.Z + 2. * k3.Z + k4.Z) / 6.
        };

        if (Result.Z >= TwoPi || Result.Z < 0.)
            Result.Z = std::fmod(Result.Z, TwoPi);

        if (Result.Z < 0.)
            Result.Z += TwoPi;

        return Result;
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Derive(point3d_t p, double mu, double forcing, double omega) noexcept
    {
        const double Position = p.X;
        const double Velocity = p.Y;
        const double Theta    = p.Z;

        return
        {
            Velocity,
            mu * (1. - Position * Position) * Velocity - Position + forcing * std::cos(Theta),
            omega
        };
    }
}

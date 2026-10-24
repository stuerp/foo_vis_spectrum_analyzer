
/** $VER: ThomasAttractor.cpp (2026.10.22) */

#include <pch.h>

#include "ThomasAttractor.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace ThomasAttractor
{
    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Thomas Cyclically Symmetric Attractor, {} theme, {} quality, {} speed, {} brightness",
            CurveThemes[_CurveThemeTo].Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

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

        deltaTime = std::clamp(deltaTime, 0., .1);

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

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);

        RenderAxes(renderTarget, 2.5);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = (_Speed == EyeCandy::Speed::Slow) ? .55 : (_Speed == EyeCandy::Speed::Fast) ? 1.75 : 1.;

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

        _AudioOrbit = ExponentialSmoothing(_AudioOrbit, Width * .34, deltaTime, 3.5);
        _AudioLift  = ExponentialSmoothing(_AudioLift, (Peak - RMS) * 1.5, deltaTime, 5.);
        _AudioScale = ExponentialSmoothing(_AudioScale, 1. + RMS * .10 + Peak * .035, deltaTime, 7.);

        World = {};
        World.Scale = { 1.,1.,1. };

        Object.Position = { 0.,_AudioLift,0. };
        Object.Rotation = { .55 + std::sin(_AnimationTime * .19) * .08 + Phase * .10,_AnimationTime * (.10 + RMS * .07) + std::sin(_AnimationTime * .13) * .16,std::sin(_AnimationTime * .23) * (.05 + Width * .15) };
        Object.Scale    = { _AudioScale,_AudioScale,_AudioScale };

        const double o = _AnimationTime * .055 + _AudioOrbit * std::sin(_AnimationTime * .31), radius = 24. - RMS * 1.5;

        Camera.Position = { std::sin(o) * radius,3. + std::sin(_AnimationTime * .11) * 1.5 + _AudioLift * .35,-std::cos(o) * radius };
        Camera.Target   = { 0.,_AudioLift,0. };
        Camera.Up       = { 0.,1.,0. };

        Projection.FieldOfView = 52. - Peak * 3.;
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
                    return 3'500;

                default:
                case Quality::Medium:
                    return 7'000;

                case Quality::High:
                    return 14'000;
            }
        }();

        const double e = Clamp((_SmoothedRMS * .55) + (_SmoothedPeak * .30) + (_PhaseEnergy * .15));
        const double b = .208186 + (e * .0045);
        const double s = .035;

        point3d_t p { .1,0.,-.1 };

        for (size_t i = 0; i < 3'500; ++i)
            p = Integrate(p, s, b);

        _Points.resize(PointCount);

        size_t Used = 0;

        for (; Used < PointCount; ++Used)
        {
            p = Integrate(p, s, b);

            if (!std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z))
                break;

            _Points[Used] = p;
        }

        _Points.resize(Used);

        GenerateSpline();
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
                const double Amount = (double) i / (double) (_SplinePoints.size() - 1);

                const double Depth     = 1. - (Previous.LinearDepth + Current.LinearDepth) * 0.5;
                const double ViewDepth =      (Previous.ViewDepth   + Current.ViewDepth)   * 0.5;

                const float Intensity = (float) (Energy * (0.52 + (Depth * 0.48)));
                const float Thickness = std::clamp(22.f / (float) std::max(ViewDepth, Projection.NearPlane), 0.55f, 2.20f);

                const uint32_t Color = GetCurveColor(Amount + (_AnimationTime * 0.025));

                DrawGlowLine(renderTarget, Previous.Position, Current.Position, Color, Intensity, Thickness);
            }

            Previous = Current;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Derive(point3d_t p, double b) noexcept
    {
        return
        {
            std::sin(p.Y) - b * p.X,
            std::sin(p.Z) - b * p.Y,
            std::sin(p.X) - b * p.Z
        };
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Integrate(point3d_t p, double s, double b) noexcept
    {
        const auto k1 = Derive(p, b);
        const auto k2 = Derive({ p.X + k1.X * s * .5, p.Y + k1.Y * s * .5, p.Z + k1.Z * s * .5 }, b);
        const auto k3 = Derive({ p.X + k2.X * s * .5, p.Y + k2.Y * s * .5, p.Z + k2.Z * s * .5 }, b);
        const auto k4 = Derive({ p.X + k3.X * s,      p.Y + k3.Y * s,      p.Z + k3.Z * s },      b);

        return
        {
            p.X + s * (k1.X + 2. * k2.X + 2. * k3.X + k4.X) / 6.,
            p.Y + s * (k1.Y + 2. * k2.Y + 2. * k3.Y + k4.Y) / 6.,
            p.Z + s * (k1.Z + 2. * k2.Z + 2. * k3.Z + k4.Z) / 6.
        };
    }
}

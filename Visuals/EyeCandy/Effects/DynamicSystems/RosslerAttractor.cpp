
/** $VER: RosslerAttractor.cpp (2026.10.22) https://en.wikipedia.org/wiki/R%C3%B6ssler_attractor */

#include <pch.h>

#include "RosslerAttractor.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <cstdint>
#include <mutex>
#include <thread>

namespace RosslerAttractor
{
    effect_t::effect_t()
    {
        StartWorkers();
    }

    effect_t::~effect_t()
    {
        StopWorkers();
    }

    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Rössler Attractor, {} theme, {} quality, {} speed, {} brightness",
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

        RenderAxes(renderTarget, 5.);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime)
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
                const float Thickness = std::clamp(28.f / (float) std::max(ViewDepth, Projection.NearPlane), 0.55f, 2.20f);

                const uint32_t Color = GetCurveColor(Amount + (_AnimationTime * 0.025));

                DrawGlowLine(renderTarget, Previous.Position, Current.Position, Color, Intensity, Thickness);
            }

            Previous = Current;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StartWorkers()
    {
        const auto WorkerCount = std::max((size_t)1, (size_t)std::thread::hardware_concurrency());

        _WorkerRanges.resize(WorkerCount);
        _Workers.reserve(WorkerCount);

        try
        {
            for (size_t i = 0; i < WorkerCount; ++i)
            {
                // Pass a closure containing an ordinary object pointer instead of a pointer-to-member function. MSVC stores std::thread arguments in an
                // internal tuple; a member-function pointer for this multiply inherited class triggers C4371 in std::_Tuple_val on newer compiler versions.
                _Workers.emplace_back([this, i]() noexcept
                {
                    Worker(i);
                });
            }
        }
        catch (...)
        {
            StopWorkers(); throw;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StopWorkers() noexcept
    {
        {
            const std::lock_guard Lock(_WorkerMutex);

            _IsStopping = true;
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        for (auto & Worker : _Workers)
        {
            if (Worker.joinable())
                 Worker.join();
        }

        _Workers.clear();
        _WorkerRanges.clear();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Worker(size_t workerIndex) noexcept
    {
        size_t Generation = 0;

        for (;;)
        {
            field_t Field;
            {
                std::unique_lock Lock(_WorkerMutex); _WorkerCondition.wait(Lock, [this, &Generation]
                {
                    return _IsStopping || _Generation != Generation;
                });

                if (_IsStopping)
                    return;

                Field = _Field;

                Generation = _Generation;
            }

            GenerateRange(workerIndex, Field);
            {
                const std::lock_guard Lock(_WorkerMutex);

                if (--_PendingWorkers == 0)
                    _CompletionCondition.notify_one();
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateRange(size_t workerIndex, const field_t & field) noexcept
    {
        const range_t Range = _WorkerRanges[workerIndex];

        point3d_t Point = Range.Seed;

        for (size_t i = Range.First; i < Range.Last; ++i)
        {
            Point = Integrate(Point, field.Step, field.A, field.B, field.C); _Points[i] = Point;
        }
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
        _AudioLift  = ExponentialSmoothing(_AudioLift, (Peak - RMS) * 2.2, deltaTime, 5.);
        _AudioScale = ExponentialSmoothing(_AudioScale, 1. + RMS * .10 + Peak * .035, deltaTime, 7.);

        const double Time = _AnimationTime;

        World = {}; World.Scale = { 1., 1., 1. };

        Object.Position = { 0., -1. + _AudioLift, 0. };
        Object.Rotation = { .43 + std::sin(Time * .19) * .08 + Phase * .10, Time * (.10 + RMS * .07) + std::sin(Time * .13) * .16, std::sin(Time * .23) * (.05 + Width * .15) };
        Object.Scale = { _AudioScale, _AudioScale, _AudioScale };

        const double Orbit = Time * .055 + _AudioOrbit * std::sin(Time * .31), Radius = 30. - RMS * 2.;

        Camera.Position = { std::sin(Orbit) * Radius, 3. + std::sin(Time * .11) * 1.8 + _AudioLift * .35, -std::cos(Orbit) * Radius };
        Camera.Target = { Object.Position.X, Object.Position.Y, 5. };
        Camera.Up = { 0., 1., 0. }; Projection.FieldOfView = 52. - Peak * 3.;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateAttractor() noexcept
    {
        if (_Workers.empty())
            return;

        const size_t PointCount = [&]() noexcept -> size_t
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low: return 1800;
                case EyeCandy::Quality::High: return 7200;
                case EyeCandy::Quality::Medium:
                default: return 3600;
            }
        }();

        const field_t Field { PointCount, .20 + (_SmoothedRMS * .055), .20 + (_PhaseEnergy * .085), 5.70 + (_SmoothedPeak * .75), .0085 };

        point3d_t Point{ .1, 0., 0. };

        for (size_t i = 0; i < 1800; ++i)
            Point = Integrate(Point, Field.Step, Field.A, Field.B, Field.C);

        _Points.resize(PointCount); const size_t Count = _Workers.size(); size_t First = 0;

        for (size_t i = 0; i < Count; ++i)
        {
            const size_t N = PointCount / Count + (i < PointCount % Count ? 1 : 0);

            auto & R = _WorkerRanges[i];

            R = { First,First + N,Point };

            for (size_t j = 0; j < N; ++j)
                Point = Integrate(Point, Field.Step, Field.A, Field.B, Field.C);

            First += N;
        }

        {
            const std::lock_guard Lock(_WorkerMutex);

            _Field = Field;

            _PendingWorkers = Count;
            ++_Generation;
        }

        _WorkerCondition.notify_all();
        {
            std::unique_lock Lock(_WorkerMutex); _CompletionCondition.wait(Lock, [this]
            {
                return _PendingWorkers == 0;
            });
        }

        const auto Invalid = std::find_if(_Points.begin(), _Points.end(), [](const point3d_t & p)
        {
            return !std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z);
        });

        _Points.erase(Invalid, _Points.end());

        GenerateSpline();
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Derive(point3d_t p, double a, double b, double c) noexcept
    {
        return { -p.Y - p.Z, p.X + a * p.Y, b + p.Z * (p.X - c) };
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Integrate(point3d_t p, double s, double a, double b, double c) noexcept
    {
        const auto k1 = Derive(p, a, b, c);
        const auto k2 = Derive({ p.X + k1.X * s * .5,p.Y + k1.Y * s * .5,p.Z + k1.Z * s * .5 }, a, b, c);
        const auto k3 = Derive({ p.X + k2.X * s * .5,p.Y + k2.Y * s * .5,p.Z + k2.Z * s * .5 }, a, b, c);
        const auto k4 = Derive({ p.X + k3.X * s,p.Y + k3.Y * s,p.Z + k3.Z * s }, a, b, c);

        return { p.X + s * (k1.X + 2. * k2.X + 2. * k3.X + k4.X) / 6., p.Y + s * (k1.Y + 2. * k2.Y + 2. * k3.Y + k4.Y) / 6., p.Z + s * (k1.Z + 2. * k2.Z + 2. * k3.Z + k4.Z) / 6. };
    }
}

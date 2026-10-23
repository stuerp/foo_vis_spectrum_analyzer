
/** $VER: LorenzAttractor.cpp (2026.10.21) https://en.wikipedia.org/wiki/Lorenz_system */

#include <pch.h>

#include "LorenzAttractor.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace LorenzAttractor
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
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept
    {
        HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = std::clamp(deltaTime, 0.0, 0.1);

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

        RenderAxes(renderTarget, 5.0);
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
                case EyeCandy::Speed::Slow: return 0.55;
                case EyeCandy::Speed::Fast: return 1.75;
                case EyeCandy::Speed::Normal:
                default: return 1.0;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        _SmoothedPeak        = ExponentialSmoothing(_SmoothedPeak,        _Peak,        deltaTime, (_Peak > _SmoothedPeak) ? 22. : 5.);
        _SmoothedRMS         = ExponentialSmoothing(_SmoothedRMS,         _RMS,         deltaTime, (_RMS  > _SmoothedRMS)  ? 16. : 4.);
        _SmoothedWidth       = ExponentialSmoothing(_SmoothedWidth,       _StereoWidth, deltaTime, 5.5);
        _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _Correlation, deltaTime, 4.0);

        UpdateCurveTheme(deltaTime);
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
    void effect_t::UpdateScene(double deltaTime) noexcept
    {
        const double Peak  = std::clamp(_SmoothedPeak,  0., 1.);
        const double RMS   = std::clamp(_SmoothedRMS,   0., 1.);
        const double Width = std::clamp(_SmoothedWidth, 0., 1.);
        const double Phase = std::clamp(_PhaseEnergy,   0., 1.);

        _AudioOrbit = ExponentialSmoothing(_AudioOrbit, Width * 0.34,                        deltaTime, 3.5);
        _AudioLift  = ExponentialSmoothing(_AudioLift, (Peak - RMS) * 5.5,                   deltaTime, 5.0);
        _AudioScale = ExponentialSmoothing(_AudioScale, 1.0 + (RMS * 0.10) + (Peak * 0.035), deltaTime, 7.0);

        const double Time = _AnimationTime;

        World = {};

        World.Scale = { 1., 1., 1. };

        Object.Position = { 0., -2. + _AudioLift, 0. };
        Object.Rotation =
        {
            0.42 + std::sin(Time * 0.19) * 0.08 + (Phase * 0.10),
            Time * (0.1 + RMS * 0.07) + std::sin(Time * 0.13) * 0.16,
            std::sin(Time * 0.23) * (0.05 + Width * 0.15)
        };
        Object.Scale = { _AudioScale, _AudioScale, _AudioScale };

        const double Orbit  = (Time * 0.055) + _AudioOrbit * std::sin(Time * 0.31);
        const double Radius = 82.0 - (RMS * 5.);

        Camera.Position =
        {
            std::sin(Orbit) * Radius,
            4.0f + std::sin(Time * 0.11f) * 3.0f + _AudioLift * 0.35f,
            -std::cos(Orbit) * Radius
        };
        Camera.Target = { Object.Position.X, Object.Position.Y + 2., 24. };
        Camera.Up = { 0., 1., 0. };

        Projection.FieldOfView = 52. - (Peak * 3.);
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
                case EyeCandy::Quality::Low:    return 1800;
                case EyeCandy::Quality::High:   return 7200;
                default:
                case EyeCandy::Quality::Medium: return 3600;
            }
        }();

        // The Lorenz system evolves one point through three coupled differential equations:
        //
        //     dx/dt = sigma * (y - x)
        //     dy/dt = x * (rho - z) - y
        //     dz/dt = x * y - beta * z
        //
        // Repeated integration produces a deterministic but chaotic trajectory around two nonzero equilibrium points when rho > 1.
        // Projecting that trajectory onto the render target creates the characteristic two-lobed, butterfly-shaped Lorenz attractor.
        const field_t Field
        {
            .PointCount = PointCount,
            .Sigma      = 10.       + (_SmoothedWidth * 0.75),
            .Rho        = 28.       + (_SmoothedPeak  * 3.),
            .Beta       = (8. / 3.) + (_PhaseEnergy   * 0.18),
            .Step       = 0.0045                                // Integration step h. Smaller values improve numerical accuracy and produce more closely spaced points; larger values advance the trajectory faster but can make it coarse or unstable.
        };

        point3d_t Point{ 0.1, 0.0, 0.0 };

        for (size_t i = 0; i < 2500; ++i)
            Point = Integrate(Point, Field.Step, Field.Sigma, Field.Rho, Field.Beta);

        _Points.resize(PointCount);

        const size_t WorkerCount = _Workers.size();

        size_t First = 0;

        for (size_t i = 0; i < WorkerCount; ++i)
        {
            const size_t Count = PointCount / WorkerCount + (i < PointCount % WorkerCount ? 1 : 0);

            range_t & Range = _WorkerRanges[i];

            Range.First = First;
            Range.Last  = First + Count;
            Range.Seed = Point;

            for (size_t j = Range.First; j < Range.Last; ++j)
                Point = Integrate(Point, Field.Step, Field.Sigma, Field.Rho, Field.Beta);

            First = Range.Last;
        }

        {
            const std::lock_guard Lock(_WorkerMutex);

            _Field = Field;

            _PendingWorkers = WorkerCount;
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        {
            std::unique_lock lock(_WorkerMutex);

            _CompletionCondition.wait(lock, [this]() noexcept
            {
                return _PendingWorkers == 0;
            });
        }

        const auto IsInvalid = std::find_if(_Points.begin(), _Points.end(), [](const point3d_t & value) noexcept
        {
            return !std::isfinite(value.X) || !std::isfinite(value.Y) || !std::isfinite(value.Z);
        });

        _Points.erase(IsInvalid, _Points.end());

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
                const double Amount = (double) i / (double)(_SplinePoints.size() - 1);
                const uint32_t Color = GetCurveColor(Amount + (_AnimationTime * 0.025));

                const double Depth     = 1. - (Previous.LinearDepth + Current.LinearDepth) * 0.5;
                const double ViewDepth = (Previous.ViewDepth + Current.ViewDepth) * 0.5f;

                const float Intensity = (float) (Energy * (0.52 + Depth * 0.48));
                const float Thickness = std::clamp(78.0f / (float) std::max(ViewDepth, Projection.NearPlane), 0.55f, 2.20f);

                DrawGlowLine(renderTarget, Previous.Position, Current.Position, Color, Intensity, Thickness);
            }

            Previous = Current;
        }
    }

    /// <summary>
    /// Start the worker threads.
    /// </summary>
    void effect_t::StartWorkers()
    {
        const auto WorkerCount = (size_t) std::max(1u, std::thread::hardware_concurrency());

        _Workers.reserve(WorkerCount);
        _WorkerRanges.resize(WorkerCount);

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
            StopWorkers();
            throw;
        }
    }

    /// <summary>
    /// Stops the worker threads.
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

        _WorkerRanges.clear();
        _Workers.clear();
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
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, &Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
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
            Point = Integrate(Point, field.Step, field.Sigma, field.Rho, field.Beta);

            _Points[i] = Point;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Lorenz Attractor, {} theme, {} quality, {} speed, {} brightness",
            CurveThemes[_CurveThemeTo].Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    ///
    /// </summary>
    effect_t::point3d_t effect_t::Derive(point3d_t point, double sigma, double rho, double beta) noexcept
    {
        return
        {
            sigma * (point.Y - point.X),
            point.X * (rho - point.Z) - point.Y,
            point.X * point.Y - beta * point.Z
        };
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Integrate(point3d_t point, double step, double sigma, double rho, double beta) noexcept
    {
        const point3d_t k1 = Derive(point, sigma, rho, beta);
        const point3d_t k2 = Derive({ point.X + k1.X * step * 0.5f, point.Y + k1.Y * step * 0.5f, point.Z + k1.Z * step * 0.5f }, sigma, rho, beta);
        const point3d_t k3 = Derive({ point.X + k2.X * step * 0.5f, point.Y + k2.Y * step * 0.5f, point.Z + k2.Z * step * 0.5f }, sigma, rho, beta);
        const point3d_t k4 = Derive({ point.X + k3.X * step, point.Y + k3.Y * step, point.Z + k3.Z * step }, sigma, rho, beta);

        return
        {
            point.X + step * (k1.X + 2.0f * k2.X + 2.0f * k3.X + k4.X) / 6.,
            point.Y + step * (k1.Y + 2.0f * k2.Y + 2.0f * k3.Y + k4.Y) / 6.,
            point.Z + step * (k1.Z + 2.0f * k2.Z + 2.0f * k3.Z + k4.Z) / 6.
        };
    }
}

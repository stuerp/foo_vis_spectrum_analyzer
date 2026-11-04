
/** $VER: ChuaCircuit.cpp (2026.11.04) Chua's Circuit double-scroll chaotic attractor. **/

#include <pch.h>

#include "ChuaCircuit.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>

namespace ChuaCircuit
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
        return std::format(L"Chua Circuit, {} theme, {} quality, {} speed, {} brightness",
            CurveThemes[_CurveThemeTo].Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        DynamicSystem::effect_t::Reset();

        _AttractorState = { .10, 0., 0. };
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

        _SmoothedPeak        = ExponentialSmoothing(_SmoothedPeak,        _Peak,        deltaTime, (_Peak > _SmoothedPeak) ? 22. : 5.0);
        _SmoothedRMS         = ExponentialSmoothing(_SmoothedRMS,         _RMS,         deltaTime, (_RMS  > _SmoothedRMS)  ? 16. : 4.0);

        _SmoothedStereoWidth = ExponentialSmoothing(_SmoothedStereoWidth, _StereoWidth, deltaTime, 5.0);
        _SmoothedCorrelation = ExponentialSmoothing(_SmoothedCorrelation, _Correlation, deltaTime, 4.0);

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
                    const double Amount = (double) i * SplinePointScale;

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
    /// Start the worker threads.
    /// </summary>
    void effect_t::StartWorkers()
    {
        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, _MaxWorkers);

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
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, Generation]() noexcept
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
            Point = Integrate(Point, field.Step, field.A, field.B, field.C);

            _Points[i] = Point;
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

        _AudioOrbit = ExponentialSmoothing(_AudioOrbit, Width * .65, deltaTime, 3.8);
        _AudioLift  = ExponentialSmoothing(_AudioLift, (Peak - RMS) * 1.25, deltaTime, 5.5);
        _AudioScale = ExponentialSmoothing(_AudioScale, 1. + RMS * .10 + Peak * .035, deltaTime, 7.);

        const double Time = _AnimationTime;

        World = {};
        World.Scale = { 1., 1., 1. };

        Object.Position = { 0., _AudioLift, 0. };
        Object.Rotation =
        {
            .18                        + std::sin(Time * .19) * .12 + Phase * .06,
            Time * (.075 + RMS * .045) + std::sin(Time * .13) * .16,
                                         std::sin(Time * .23) * (.06 + Width * .10)
        };
        Object.Scale = { _AudioScale, _AudioScale, _AudioScale };

        const double Orbit = Time * .038 + _AudioOrbit * std::sin(Time * .24);
        const double Radius = 27. - RMS * 1.8;

        Camera.Position =
        {
                  std::sin(Orbit) * Radius,
            3.0 + std::sin(Time * .11) * 1.8 + _AudioLift * .25,
                 -std::cos(Orbit) * Radius
        };
        Camera.Target = { 0., 0., 0. };
        Camera.Up = { 0., 1., 0. };

        Projection.FieldOfView = 48. - Peak * 2.5;
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
                case EyeCandy::Quality::Low:
                    return 3000;

                default:
                case EyeCandy::Quality::Medium:
                    return 6000;

                case EyeCandy::Quality::High:
                    return 12000;
            }
        }();

        // Normalized Chua system. Audio stays close to the stable double-scroll region:
        // alpha controls lobe switching, beta controls vertical folding, and C morphs the inner/outer slopes without changing their dissipative ordering.
        const double Alpha      = std::clamp(15.6 + (_SmoothedRMS * 1.25) + (_SmoothedPeak * .35),       15.2, 17.1);
        const double Beta       = std::clamp(28.0 + (_SmoothedStereoWidth * 2.2) - (_PhaseEnergy * .65), 26.5, 31.0);
        const double SlopeMorph = std::clamp((_SmoothedPeak * .55) + (_SmoothedRMS * .25),                0.0, 0.72);

        const field_t Field { PointCount, Alpha, Beta, SlopeMorph, .0045 };

        // Remove the startup transient before rendering the invariant double scroll.
        if (!_AttractorSettled)
        {
            for (size_t i = 0; i < 5000; ++i)
                _AttractorState = Integrate(_AttractorState, Field.Step, Field.A, Field.B, Field.C);

            _AttractorSettled = true;
        }

        _Points.resize(PointCount);

        const size_t WorkerCount = _Workers.size();

        size_t First = 0;

        for (size_t i = 0; i < WorkerCount; ++i)
        {
            const size_t N = PointCount / WorkerCount + (i < PointCount % WorkerCount ? 1 : 0);

            auto & Range = _WorkerRanges[i];

            Range = { First, First + N, _AttractorState };

            for (size_t j = 0; j < N; ++j)
                _AttractorState = Integrate(_AttractorState, Field.Step, Field.A, Field.B, Field.C);

            First += N;
        }

        {
            const std::lock_guard Lock(_WorkerMutex);

            _Field = Field;

            _PendingWorkers = WorkerCount;
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        {
            std::unique_lock Lock(_WorkerMutex);

            _CompletionCondition.wait(Lock, [this]
            {
                return _PendingWorkers == 0;
            });
        }

        const auto Invalid = std::find_if(_Points.begin(), _Points.end(), [](const point3d_t & p)
        {
            return !std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z);
        });

        _Points.erase(Invalid, _Points.end());

        // Normalize Chua's compact electrical state for the shared 3D scene.
        for (auto & Point : _Points)
        {
            Point.X *= 5.7;
            Point.Y *= 9.0;
            Point.Z *= 3.1;
        }

        GenerateSpline();
    }

    /// <summary>
    /// Integrates the equation using fourth-order Runge-Kutta. s = integration step, a = alpha, b = beta, c = audio-reactive slope morph
    /// </summary>
    effect_t::point3d_t effect_t::Integrate(point3d_t p, double s, double a, double b, double c) noexcept
    {
        const auto k1 = Derive(p, a, b, c);
        const auto k2 = Derive({ p.X + k1.X * s * .5, p.Y + k1.Y * s * .5, p.Z + k1.Z * s * .5 }, a, b, c);
        const auto k3 = Derive({ p.X + k2.X * s * .5, p.Y + k2.Y * s * .5, p.Z + k2.Z * s * .5 }, a, b, c);
        const auto k4 = Derive({ p.X + k3.X * s,      p.Y + k3.Y * s,      p.Z + k3.Z * s },      a, b, c);

        return
        {
            p.X + s * (k1.X + 2. * k2.X + 2. * k3.X + k4.X) / 6.,
            p.Y + s * (k1.Y + 2. * k2.Y + 2. * k3.Y + k4.Y) / 6.,
            p.Z + s * (k1.Z + 2. * k2.Z + 2. * k3.Z + k4.Z) / 6.
        };
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Derive(point3d_t p, double alpha, double beta, double slopeMorph) noexcept
    {
        // Piecewise-linear Chua diode characteristic: h(x) = m1 * x + .5 * (m0 - m1) * (|x+1| - |x-1|)
        const double InnerSlope = -1.143 - slopeMorph * .055;
        const double OuterSlope =  -.714 - slopeMorph * .030;
        const double Diode      = OuterSlope * p.X + .5 * (InnerSlope - OuterSlope) * (std::abs(p.X + 1.) - std::abs(p.X - 1.));

        return
        {
            alpha * (p.Y - p.X - Diode),
            p.X - p.Y + p.Z,
            -beta * p.Y
        };
    }
}

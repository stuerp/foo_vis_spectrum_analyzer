
/** $VER: HalvorsenAttractor.cpp (2026.10.20) https://en.wikipedia.org/wiki/Halvorsen_attractor */

#include <pch.h>

#include "HalvorsenAttractor.h"

#include <algorithm>
#include <condition_variable>
#include <cmath>
#include <cstdint>
#include <format>
#include <mutex>
#include <thread>

namespace HalvorsenAttractor
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
        return std::format(L"Halvorsen Attractor, {} theme, {} quality, {} speed, {} brightness",
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

        RenderAxes(renderTarget, 5.);
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
    void effect_t::RenderCurve(ID2D1RenderTarget * renderTarget) noexcept
    {
        if ((_Width < 2.f) || (_Height < 2.f) || (_SplinePoints.size() < 2))
            return;

        const double Energy = std::clamp(0.32 + _SmoothedRMS * 2.2 + _SmoothedPeak * 0.35, 0.32, 1.);

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
                const uint32_t Color = GetCurveColor(Amount + _AnimationTime * 0.025);

                const double Depth = 1. - (Previous.LinearDepth + Current.LinearDepth) * 0.5;
                const double ViewDepth = (Previous.ViewDepth + Current.ViewDepth) * 0.5;

                const float Intensity = (float) (Energy * (0.52 + Depth * 0.48));
                const float Thickness = std::clamp(34.f / (float) std::max(ViewDepth, Projection.NearPlane), 0.55f, 2.20f);

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
            Point = Integrate(Point, field.Step, field.A);
            _Points[i] = Point;
        }
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

        _AudioOrbit = ExponentialSmoothing(_AudioOrbit, Width * 0.34,                    deltaTime, 3.5);
        _AudioLift  = ExponentialSmoothing(_AudioLift, (Peak - RMS) * 2.0,               deltaTime, 5.0);
        _AudioScale = ExponentialSmoothing(_AudioScale, 1.0 + RMS * 0.10 + Peak * 0.035, deltaTime, 7.0);

        const double Time = _AnimationTime;

        World = {};
        World.Scale = { 1., 1., 1. };

        Object.Position = { 0., -1.0 + _AudioLift, 0. };
        Object.Rotation =
        {
            0.52 + std::sin(Time * 0.19) * 0.08 + Phase * 0.10,
            Time * (0.10 + RMS * 0.07) + std::sin(Time * 0.13) * 0.16,
            std::sin(Time * 0.23) * (0.05 + Width * 0.15)
        };
        Object.Scale = { _AudioScale, _AudioScale, _AudioScale };

        const double Orbit = Time * 0.055 + _AudioOrbit * std::sin(Time * 0.31);
        const double Radius = 38.0 - RMS * 3.0;

        Camera.Position =
        {
            std::sin(Orbit) * Radius,
            4.0 + std::sin(Time * 0.11) * 2.0 + _AudioLift * 0.35,
            -std::cos(Orbit) * Radius
        };
        Camera.Target = { Object.Position.X, Object.Position.Y, 0. };
        Camera.Up = { 0., 1., 0. };

        Projection.FieldOfView = 52. - Peak * 3.;
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
                case EyeCandy::Quality::Low: return 3000;
                case EyeCandy::Quality::High: return 12000;
                case EyeCandy::Quality::Medium:
                default: return 6000;
            }
        }();

        const double AudioEnergy = std::clamp(_SmoothedRMS * 0.65 + _SmoothedPeak * 0.25 + _PhaseEnergy * 0.10, 0.0, 1.0);

        const field_t Field
        {
            .PointCount = PointCount,
            .A = 1.40 + AudioEnergy * 0.045,
            .Step = 0.0045
        };

        point3d_t Point { -5., 0., 0. };

        for (size_t i = 0; i < 5000; ++i)
            Point = Integrate(Point, Field.Step, Field.A);

        _Points.resize(PointCount);

        const size_t WorkerCount = _Workers.size();

        size_t First = 0;

        for (size_t i = 0; i < WorkerCount; ++i)
        {
            const size_t Count = PointCount / WorkerCount + (i < PointCount % WorkerCount ? 1 : 0);
            range_t & Range = _WorkerRanges[i];

            Range.First = First;
            Range.Last  = First + Count;
            Range.Seed  = Point;

            for (size_t j = Range.First; j < Range.Last; ++j)
                Point = Integrate(Point, Field.Step, Field.A);

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
            std::unique_lock Lock(_WorkerMutex);

            _CompletionCondition.wait(Lock, [this]() noexcept
            {
                return _PendingWorkers == 0;
            });
        }

        const auto IsInvalid = std::find_if(_Points.begin(), _Points.end(), [](const point3d_t & Value) noexcept
        {
            return !std::isfinite(Value.X) || !std::isfinite(Value.Y) || !std::isfinite(Value.Z);
        });

        _Points.erase(IsInvalid, _Points.end());
        GenerateSpline();
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Derive(point3d_t point, double a) noexcept
    {
        return
        {
            -a * point.X - 4. * point.Y - 4. * point.Z - point.Y * point.Y,
            -a * point.Y - 4. * point.Z - 4. * point.X - point.Z * point.Z,
            -a * point.Z - 4. * point.X - 4. * point.Y - point.X * point.X
        };
    }

    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::Integrate(point3d_t point, double step, double a) noexcept
    {
        const point3d_t k1 = Derive(point, a);
        const point3d_t k2 = Derive({ point.X + k1.X * step * 0.5, point.Y + k1.Y * step * 0.5, point.Z + k1.Z * step * 0.5 }, a);
        const point3d_t k3 = Derive({ point.X + k2.X * step * 0.5, point.Y + k2.Y * step * 0.5, point.Z + k2.Z * step * 0.5 }, a);
        const point3d_t k4 = Derive({ point.X + k3.X * step, point.Y + k3.Y * step, point.Z + k3.Z * step }, a);

        return
        {
            point.X + step * (k1.X + 2. * k2.X + 2. * k3.X + k4.X) / 6.,
            point.Y + step * (k1.Y + 2. * k2.Y + 2. * k3.Y + k4.Y) / 6.,
            point.Z + step * (k1.Z + 2. * k2.Z + 2. * k3.Z + k4.Z) / 6.
        };
    }
/*
    /// <summary>
    /// 
    /// </summary>
    effect_t::point3d_t effect_t::EvaluateSpline(point3d_t p0, point3d_t p1, point3d_t p2, point3d_t p3, double amount) noexcept
    {
        const double Amount2 = amount * amount;
        const double Amount3 = Amount2 * amount;

        const double Weight0 = (1. - 3. * amount + 3. * Amount2 - Amount3) / 6.;
        const double Weight1 = (4. - 6. * Amount2 + 3. * Amount3) / 6.;
        const double Weight2 = (1. + 3. * amount + 3. * Amount2 - 3. * Amount3) / 6.;
        const double Weight3 = Amount3 / 6.;

        return
        {
            p0.X * Weight0 + p1.X * Weight1 + p2.X * Weight2 + p3.X * Weight3,
            p0.Y * Weight0 + p1.Y * Weight1 + p2.Y * Weight2 + p3.Y * Weight3,
            p0.Z * Weight0 + p1.Z * Weight1 + p2.Z * Weight2 + p3.Z * Weight3
        };
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateSpline() noexcept
    {
        _SplinePoints.clear();

        if (_Points.size() < 2)
            return;

        const size_t SubdivisionCount = std::clamp(_SplineSubdivision, (size_t) 1, (size_t) 12);
        const double Smoothing = std::clamp(_SplineSmoothing, 0., 1.);

        _SplinePoints.reserve((_Points.size() - 1) * SubdivisionCount + 1);
        _SplinePoints.push_back(_Points.front());

        for (size_t i = 0; i + 1 < _Points.size(); ++i)
        {
            const point3d_t p0 = _Points[i > 0 ? i - 1 : i];
            const point3d_t p1 = _Points[i];
            const point3d_t p2 = _Points[i + 1];
            const point3d_t p3 = _Points[i + 2 < _Points.size() ? i + 2 : i + 1];

            for (size_t j = 1; j <= SubdivisionCount; ++j)
            {
                const double Amount = (double) j / (double) SubdivisionCount;
                const point3d_t Spline = EvaluateSpline(p0, p1, p2, p3, Amount);
                const point3d_t Linear
                {
                    Lerp(p1.X, p2.X, Amount),
                    Lerp(p1.Y, p2.Y, Amount),
                    Lerp(p1.Z, p2.Z, Amount)
                };

                _SplinePoints.push_back
                ({
                    Lerp(Linear.X, Spline.X, Smoothing),
                    Lerp(Linear.Y, Spline.Y, Smoothing),
                    Lerp(Linear.Z, Spline.Z, Smoothing)
                });
            }
        }
    }
*/
}

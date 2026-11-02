
/** $VER: HiddenLineLandscape.cpp (2026.11.02) */

#include <pch.h>

#include "HiddenLineLandscape.h"

#include <algorithm>
#include <cmath>
#include <format>


namespace HiddenLineLandscape
{
    /// <summary>
    ///
    /// </summary>
    effect_t::~effect_t()
    {
        StopWorkers();
    }
    /// <summary>
    /// Creates persistent workers for the CPU depth prepass.
    /// </summary>
    void effect_t::InitializeWorkers()
    {
        if (!_Workers.empty())
            return;

        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, 8);

        _IsStopping = false;

        _Workers.reserve(WorkerCount);

        for (size_t WorkerIndex = 0; WorkerIndex < WorkerCount; ++WorkerIndex)
        {
            _Workers.emplace_back([this, WorkerIndex]() noexcept
            {
                WorkerLoop(WorkerIndex);
            });
        }
    }

    /// <summary>
    /// Stops and joins the persistent depth workers.
    /// </summary>
    void effect_t::StopWorkers() noexcept
    {
        {
            const std::lock_guard Lock(_WorkerMutex);

            _IsStopping = true;
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        for (std::thread & Worker : _Workers)
        {
            if (Worker.joinable())
                Worker.join();
        }

        _Workers.clear();
        _IsStopping = false;
    }

    /// <summary>
    /// Waits for depth-prepass generations dispatched by the render thread.
    /// </summary>
    void effect_t::WorkerLoop(size_t workerIndex) noexcept
    {
        size_t LocalGeneration = 0;
        for (;;)
        {
            {
                std::unique_lock Lock(_WorkerMutex);
                _WorkerCondition.wait(Lock, [this, &LocalGeneration]() noexcept
                {
                    return _IsStopping || (_Generation != LocalGeneration);
                });
                if (_IsStopping)
                    return;
                LocalGeneration = _Generation;
            }
            if (workerIndex < _ActiveWorkerCount)
                RasterizeDepthBand(workerIndex);
            {
                const std::lock_guard Lock(_WorkerMutex);
                if ((workerIndex < _ActiveWorkerCount) && (--_PendingWorkers == 0))
                    _CompletionCondition.notify_one();
            }
        }
    }

    /// <summary>
    /// Dispatches independent horizontal depth-buffer bands and waits for completion.
    /// </summary>
    void effect_t::DispatchDepthPrepass(size_t rowCount, size_t samplesPerRow, size_t width, size_t height) noexcept
    {
        InitializeWorkers();
        _DepthRowCount = rowCount;
        _DepthSamplesPerRow = samplesPerRow;
        _DepthBufferWidth = width;
        _DepthBufferHeight = height;
        _ActiveWorkerCount = std::min(_Workers.size(), height);
        if (_ActiveWorkerCount == 0)
            return;
        {
            const std::lock_guard Lock(_WorkerMutex);
            _PendingWorkers = _ActiveWorkerCount;
            ++_Generation;
        }
        _WorkerCondition.notify_all();
        std::unique_lock Lock(_WorkerMutex);
        _CompletionCondition.wait(Lock, [this]() noexcept
        {
            return _PendingWorkers == 0;
        });
    }

    /// <summary>
    /// Rasterizes all terrain triangles clipped to one non-overlapping horizontal band.
    /// </summary>
    void effect_t::RasterizeDepthBand(size_t workerIndex) noexcept
    {
        const size_t FirstY = (_DepthBufferHeight * workerIndex) / _ActiveWorkerCount;
        const size_t LastY = (_DepthBufferHeight * (workerIndex + 1)) / _ActiveWorkerCount;
        const projected_point_t * const Grid = _ProjectedGrid.data();
        for (size_t Row = 0; (Row + 1) < _DepthRowCount; ++Row)
        {
            for (size_t Sample = 0; (Sample + 1) < _DepthSamplesPerRow; ++Sample)
            {
                const projected_point_t & A = Grid[(Row * _DepthSamplesPerRow) + Sample];
                const projected_point_t & B = Grid[(Row * _DepthSamplesPerRow) + Sample + 1];
                const projected_point_t & C = Grid[((Row + 1) * _DepthSamplesPerRow) + Sample];
                const projected_point_t & D = Grid[((Row + 1) * _DepthSamplesPerRow) + Sample + 1];
                RasterizeDepthTriangle(A, B, C, _DepthBufferWidth, _DepthBufferHeight, FirstY, LastY);
                RasterizeDepthTriangle(B, D, C, _DepthBufferWidth, _DepthBufferHeight, FirstY, LastY);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Hidden-line landscape, {} quality, {} speed", Qualities[(size_t)_Quality], Speeds[(size_t)_Speed]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);
        if (FAILED(hr))
            return;
        deltaTime = Clamp(deltaTime);
        AnalyzeAudio(chunk);
        Update(deltaTime);
        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        RenderBackground(renderTarget);
        RenderHorizonGlow(renderTarget);
        RenderLandscape(renderTarget);
        RenderScanlines(renderTarget);
        // A thin monochrome frame reinforces the appearance of an early vector display and hides clipped line ends at the viewport edge.
        RenderFrame(renderTarget, D2D1::ColorF(0.60f, 1.00f, 0.72f, 0.20f + (float)_BeatPulse * 0.22f));
    }

    /// <summary>
    /// Analyzes an audio chunk.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _Peak = 0.;
        _RMS = 0.;
        _BassEnergy = 0.;
        _MidEnergy = 0.;
        _HasAudio = false;
        if (chunk.is_empty())
            return;
        const size_t FrameCount = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();
        const size_t SampleRate = chunk.get_sample_rate();
        const size_t MaxFrames = std::min<size_t>(FrameCount, 4096);
        const size_t FrameIndex = FrameCount - MaxFrames;
        if ((MaxFrames < 8) || (ChannelCount == 0) || (SampleRate == 0))
            return;
        const audio_sample * Frame = chunk.get_data() + (FrameIndex * ChannelCount);
        const audio_sample * const EndFrame = Frame + (MaxFrames * ChannelCount);
        double Sum = 0.;
        double SumBass = 0.;
        double SumMid = 0.;
        const double SubBassCoefficient = std::exp(-TwoPi * SubBassCutoff / (double)SampleRate);
        const double BassCoefficient = std::exp(-TwoPi * BassCutoff / (double)SampleRate);
        const double MidCoefficient = std::exp(-TwoPi * MidCutoff / (double)SampleRate);
        const double ChannelScale = 1. / (double)ChannelCount;
        while (Frame < EndFrame)
        {
            double Mono = 0.;
            const audio_sample * Channel = Frame;
            const audio_sample * const ChannelEnd = Frame + ChannelCount;
            while (Channel < ChannelEnd)
                Mono += *Channel++;
            Mono = ClampAudio(Mono * ChannelScale);
            _Peak = std::max(_Peak, std::abs(Mono));
            Sum += Mono * Mono;
            _BassState = (1. - BassCoefficient) * Mono + (BassCoefficient * _BassState);
            _SubBassState = (1. - SubBassCoefficient) * _BassState + (SubBassCoefficient * _SubBassState);
            _MidState = (1. - MidCoefficient) * Mono + (MidCoefficient * _MidState);
            const double Bass = _BassState - _SubBassState;
            const double Mid = _MidState - _BassState;
            SumBass += Bass * Bass;
            SumMid += Mid * Mid;
            Frame += ChannelCount;
        }
        const double FramesScale = 1. / (double)MaxFrames;
        _RMS = std::sqrt(Sum * FramesScale);
        _BassEnergy = std::sqrt(SumBass * FramesScale);
        _MidEnergy = std::sqrt(SumMid * FramesScale);
        _BassEnergy = Clamp(std::log1p(_BassEnergy * 36.) / std::log1p(36.));
        _MidEnergy = Clamp(std::log1p(_MidEnergy * 18.) / std::log1p(18.));
        _HasAudio = (_Peak > 1.0e-5) || (_RMS > 1.0e-6);
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
                case EyeCandy::Speed::Slow:
                    return 0.70;
                case EyeCandy::Speed::Fast:
                    return 1.85;
                case EyeCandy::Speed::Normal:
                default:
                    return 1.00;
            }
        }();
        _AnimationTime += deltaTime * SpeedFactor;
        _SmoothedPeak = ExponentialSmoothing(_SmoothedPeak, _Peak, deltaTime, (_Peak > _SmoothedPeak) ? 34.0 : 6.5);
        _SmoothedRMS = ExponentialSmoothing(_SmoothedRMS, _RMS, deltaTime, (_RMS > _SmoothedRMS) ? 26.0 : 5.5);
        _SmoothedBass = ExponentialSmoothing(_SmoothedBass, _BassEnergy, deltaTime, (_BassEnergy > _SmoothedBass) ? 42.0 : 12.0);
        _SmoothedMid = ExponentialSmoothing(_SmoothedMid, _MidEnergy, deltaTime, (_MidEnergy > _SmoothedMid) ? 8.0 : 3.0);
        _BeatAverage = ExponentialSmoothing(_BeatAverage, _BassEnergy, deltaTime, (_BassEnergy < _BeatAverage) ? 5.0 : 0.85);
        const double BassRise = std::max(0., _BassEnergy - _PreviousBass);
        const double BassAboveAverage = std::max(0., _SmoothedBass - _BeatAverage);
        const double AttackStrength = Clamp((BassAboveAverage * 2.5) + (BassRise * 9.0));
        _PreviousBass = _BassEnergy;
        _BeatCooldown = std::max(0., _BeatCooldown - deltaTime);
        _PeakSpawnCooldown = std::max(0., _PeakSpawnCooldown - deltaTime);
        _BeatPulse *= std::exp(-deltaTime * 10.0);
        if (_HasAudio && (_BeatCooldown <= 0.) && (BassRise > 0.008) && (AttackStrength > 0.10))
        {
            _BeatPulse = std::max(_BeatPulse, AttackStrength);
            _BeatCooldown = 0.085;
            if (_PeakSpawnCooldown <= 0.)
            {
                SpawnPeak(AttackStrength);
                _PeakSpawnCooldown = 0.11;
            }
        }
        // Move only while audio is present. The short release avoids visible start/stop jitter between quiet chunks.
        const double MotionTarget = _HasAudio ? 1. : 0.;
        _AudioMotion = ExponentialSmoothing(_AudioMotion, MotionTarget, deltaTime, _HasAudio ? 14.0 : 4.0);
        const double TravelSpeed = (0.72 + (_SmoothedRMS * 1.10) + (_BeatPulse * 0.52)) * SpeedFactor * _AudioMotion;
        _TerrainTravel += TravelSpeed * deltaTime;
        _CameraSway = ExponentialSmoothing(_CameraSway, std::sin(_AnimationTime * 0.19) * (0.10 + _SmoothedMid * 0.32), deltaTime, 1.8);
        // Follow the actual slope on the camera path. Dense forward samples prevent a narrow peak from falling between probes.
        constexpr double BaseCameraHeight = 1.45;
        constexpr double CameraClearance = 0.38;
        double RequiredCameraHeight = BaseCameraHeight;
        const double CameraX = -_CameraSway;
        for (double Z = 0.35; Z <= 7.50; Z += 0.18)
        {
            const double DistanceWeight = 1. - std::clamp((Z - 0.35) / 7.15, 0., 1.);
            const double PathHeight = std::max({ TerrainHeight(CameraX - 0.42, Z), TerrainHeight(CameraX, Z), TerrainHeight(CameraX + 0.42, Z) });
            const double AnticipatedHeight = BaseCameraHeight + ((PathHeight + CameraClearance - BaseCameraHeight) * (0.40 + (0.60 * DistanceWeight)));
            RequiredCameraHeight = std::max(RequiredCameraHeight, AnticipatedHeight);
        }
        // Also inspect each approaching peak analytically. This guarantees enough clearance even before its sampled slope reaches the near field.
        for (const peak_t & Peak : _Peaks)
        {
            const double TrailingExtent = Peak.Width * 3.50;
            if ((Peak.Z < -TrailingExtent) || (Peak.Z > 8.50))
                continue;
            const double LateralDistance = std::abs(CameraX - Peak.X);
            const double LateralReach = Peak.Width * 1.35;
            if (LateralDistance > LateralReach)
                continue;
            const double LateralInfluence = 1. - (LateralDistance / LateralReach);
            const double PeakTop = Peak.Height * Peak.Strength * LateralInfluence;
            const double Approach = 1. - std::clamp((Peak.Z - 0.35) / 8.15, 0., 1.);
            // Fade the camera support only after the peak center has passed, matching the Gaussian trailing slope.
            const double TrailingT = std::clamp((Peak.Z + TrailingExtent) / TrailingExtent, 0., 1.);
            const double TrailingInfluence = TrailingT * TrailingT * (3. - (2. * TrailingT));
            const double AnticipatedHeight = BaseCameraHeight + std::max(0., PeakTop + CameraClearance - BaseCameraHeight) + (PeakTop * 0.72 * Approach * TrailingInfluence);
            RequiredCameraHeight = std::max(RequiredCameraHeight, AnticipatedHeight);
        }
        const double TargetElevation = std::clamp(RequiredCameraHeight - BaseCameraHeight, 0., 2.10);
        const double CameraResponse = (TargetElevation > _CameraElevation) ? 11.0 : 2.2;
        _CameraElevation = ExponentialSmoothing(_CameraElevation, TargetElevation, deltaTime, CameraResponse);
        // Peaks are born near the horizon and move toward the camera with the scrolling terrain.
        for (peak_t & Peak : _Peaks)
            Peak.Z -= TravelSpeed * deltaTime;
        _Peaks.erase(std::remove_if(_Peaks.begin(), _Peaks.end(), [](const peak_t & peak) noexcept
        {
            // Do not remove a mountain when its center crosses the camera. Retain its full trailing slope.
            const double TrailingExtent = peak.Width * 3.50;
            return peak.Z < -TrailingExtent;
        }), _Peaks.end());
    }

    /// <summary>
    /// Creates a mountain peak near the horizon. Forward terrain motion carries it toward the viewer.
    /// </summary>
    void effect_t::SpawnPeak(double strength) noexcept
    {
        _RandomState = Hash(_RandomState + 0x9E3779B9u);
        peak_t Peak;
        Peak.X = RandomSigned(_RandomState) * 4.8;
        Peak.Z = 12.5;
        Peak.Height = 0.28 + (strength * 0.95) + (Random(_RandomState) * 0.24);
        Peak.Width = 0.95 + (Random(_RandomState) * 1.20);
        Peak.Strength = 0.68 + (Random(_RandomState) * 0.20);
        _Peaks.push_back(Peak);
    }

    /// <summary>
    /// Returns the deterministic base terrain plus the temporary peaks created by bass transients.
    /// </summary>
    double effect_t::TerrainHeight(double x, double z) const noexcept
    {
        // Keep the grid itself level. Only discrete beat-generated peaks deform the surface.
        if (!_HasAudio)
            return 0.;
        double Height = 0.;
        for (const peak_t & Peak : _Peaks)
        {
            const double DX = (x - Peak.X) / Peak.Width;
            const double DZ = (z - Peak.Z) / (Peak.Width * 1.85);
            const double Radius = (DX * DX) + (DZ * DZ);
            Height += Peak.Height * Peak.Strength * std::exp(-Radius * 1.85);
        }
        return Height;
    }

    /// <summary>
    /// Projects a terrain vertex using the deliberately simple perspective of early computer landscapes.
    /// </summary>
    effect_t::projected_point_t effect_t::Project(double x, double y, double z) const noexcept
    {
        projected_point_t Result;
        if (z <= 0.10)
            return Result;
        const double Horizon = _Height * 0.40;
        const double FocalLength = std::min(_Width, _Height) * 0.92;
        Result.Position.x = (float)(_CenterX + (((x + _CameraSway) / z) * FocalLength));
        Result.Position.y = (float)(Horizon + (((1.45 + _CameraElevation - y) / z) * FocalLength));
        Result.InverseDepth = (float)(1. / z);
        Result.IsVisible = std::isfinite(Result.Position.x) && std::isfinite(Result.Position.y);
        return Result;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();
        _BassState = 0.;
        _SubBassState = 0.;
        _MidState = 0.;
        _PreviousBass = 0.;
        _BeatAverage = 0.;
        _BeatPulse = 0.;
        _BeatCooldown = 0.;
        _PeakSpawnCooldown = 0.;
        _TerrainTravel = 0.;
        _CameraSway = 0.;
        _CameraElevation = 0.;
        _AudioMotion = 0.;
        _Peaks.clear();
        _DepthBuffer.clear();
        _ProjectedGrid.clear();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderBackground(ID2D1RenderTarget * renderTarget) noexcept
    {
        renderTarget->Clear(D2D1::ColorF(0.001f, 0.006f, 0.003f, 1.f));
    }

    /// <summary>
    /// Draws a restrained glow around the vanishing line without changing the monochrome character.
    /// </summary>
    void effect_t::RenderHorizonGlow(ID2D1RenderTarget * renderTarget) noexcept
    {
        const float Horizon = _Height * 0.40f;
        const float Pulse = (float)Clamp(_BeatPulse);
        for (size_t i = 0; i < 5; ++i)
        {
            const float Offset = (float)i * 2.f;
            _GlowBrush->SetColor(D2D1::ColorF(0.24f, 0.92f, 0.48f, (0.038f - (float)i * 0.006f) + Pulse * 0.012f));
            renderTarget->DrawLine(D2D1::Point2F(0.f, Horizon + Offset), D2D1::Point2F(_Width, Horizon + Offset), _GlowBrush.Get(), 1.f);
        }
    }

    /// <summary>
    /// Renders front-to-back terrain rows. A per-pixel upper envelope removes line sections hidden by rows already drawn in front.
    /// </summary>
    void effect_t::RenderLandscape(ID2D1RenderTarget * renderTarget) noexcept
    {
        const size_t BufferWidth = std::max<size_t>(2, (size_t)std::ceil(_Width));
        const size_t BufferHeight = std::max<size_t>(2, (size_t)std::ceil(_Height));
        const size_t DepthBufferSize = BufferWidth * BufferHeight;
        _DepthBuffer.resize(DepthBufferSize);
        std::fill(_DepthBuffer.begin(), _DepthBuffer.end(), 0.f);
        const size_t RowCount = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low:
                    return size_t{ 46 };
                case EyeCandy::Quality::High:
                    return size_t{ 112 };
                case EyeCandy::Quality::Medium:
                default:
                    return size_t{ 76 };
            }
        }();
        const size_t SamplesPerRow = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low:
                    return size_t{ 92 };
                case EyeCandy::Quality::High:
                    return size_t{ 240 };
                case EyeCandy::Quality::Medium:
                default:
                    return size_t{ 156 };
            }
        }();
        const double NearZ = 0.35;
        const double FarZ = 13.5;
        const double XRange = 7.4;
        const size_t GridSize = RowCount * SamplesPerRow;
        _ProjectedGrid.resize(GridSize);
        projected_point_t * const Grid = _ProjectedGrid.data();
        for (size_t Row = 0; Row < RowCount; ++Row)
        {
            const double RowT = (double)Row / (double)(RowCount - 1);
            const double Z = NearZ + (RowT * RowT * (FarZ - NearZ));
            for (size_t Sample = 0; Sample < SamplesPerRow; ++Sample)
            {
                const double T = (double)Sample / (double)(SamplesPerRow - 1);
                const double X = -XRange + (T * XRange * 2.);
                Grid[(Row * SamplesPerRow) + Sample] = Project(X, TerrainHeight(X, Z), Z);
            }
        }
        // The terrain surface is rasterized into independent horizontal depth-buffer bands before any wireframe lines are drawn.
        DispatchDepthPrepass(RowCount, SamplesPerRow, BufferWidth, BufferHeight);
        const float Pulse = (float)Clamp(_BeatPulse);
        const float RowStrokeWidth = 0.75f + Pulse * 0.35f;
        _LandscapeBrush->SetColor(D2D1::ColorF(0.55f + Pulse * 0.20f, 1.00f, 0.68f + Pulse * 0.17f, 0.86f));
        for (size_t Row = 0; Row < RowCount; ++Row)
        {
            for (size_t Sample = 0; (Sample + 1) < SamplesPerRow; ++Sample)
                DrawDepthClippedLine(renderTarget, Grid[(Row * SamplesPerRow) + Sample], Grid[(Row * SamplesPerRow) + Sample + 1], RowStrokeWidth, BufferWidth, BufferHeight);
        }
        const size_t LongitudeStep = std::max<size_t>(1, SamplesPerRow / ((_Quality == EyeCandy::Quality::High) ? 35 : (_Quality == EyeCandy::Quality::Low ? 17 : 25)));
        for (size_t Sample = 0; Sample < SamplesPerRow; Sample += LongitudeStep)
        {
            for (size_t Row = 0; (Row + 1) < RowCount; ++Row)
                DrawDepthClippedLine(renderTarget, Grid[(Row * SamplesPerRow) + Sample], Grid[((Row + 1) * SamplesPerRow) + Sample], 0.58f, BufferWidth, BufferHeight);
        }
    }

    /// <summary>
    /// Rasterizes a projected terrain triangle into an inverse-depth buffer. Larger values are closer to the viewer.
    /// </summary>
    void effect_t::RasterizeDepthTriangle(const projected_point_t & a, const projected_point_t & b, const projected_point_t & c, size_t width, size_t height, size_t firstY, size_t lastY) noexcept
    {
        if (!a.IsVisible || !b.IsVisible || !c.IsVisible)
            return;
        const float MinXF = std::min({ a.Position.x, b.Position.x, c.Position.x });
        const float MaxXF = std::max({ a.Position.x, b.Position.x, c.Position.x });
        const float MinYF = std::min({ a.Position.y, b.Position.y, c.Position.y });
        const float MaxYF = std::max({ a.Position.y, b.Position.y, c.Position.y });
        // Reject off-screen cells before calculating edge equations or touching the depth buffer.
        if ((MaxXF < 0.f) || (MaxYF < (float)firstY) || (MinXF >= (float)width) || (MinYF >= (float)lastY))
            return;
        const float Area = ((b.Position.x - a.Position.x) * (c.Position.y - a.Position.y)) - ((b.Position.y - a.Position.y) * (c.Position.x - a.Position.x));
        if (std::abs(Area) < 1.0e-5f)
            return;
        const int MinX = std::clamp((int)std::floor(MinXF), 0, (int)width - 1);
        const int MaxX = std::clamp((int)std::ceil(MaxXF), 0, (int)width - 1);
        const int MinY = std::clamp((int)std::floor(MinYF), (int)firstY, (int)lastY - 1);
        const int MaxY = std::clamp((int)std::ceil(MaxYF), (int)firstY, (int)lastY - 1);
        if (MinY > MaxY)
            return;
        const float Orientation = (Area < 0.f) ? -1.f : 1.f;
        const float InverseArea = Orientation / Area;
        // Edge equations and inverse depth are advanced incrementally. The inner loop contains additions and comparisons only.
        const float EdgeA_DX = Orientation * (b.Position.y - c.Position.y);
        const float EdgeA_DY = Orientation * (c.Position.x - b.Position.x);
        const float EdgeB_DX = Orientation * (c.Position.y - a.Position.y);
        const float EdgeB_DY = Orientation * (a.Position.x - c.Position.x);
        const float StartX = (float)MinX + 0.5f;
        const float StartY = (float)MinY + 0.5f;
        float EdgeA_Row = Orientation * (((b.Position.x - StartX) * (c.Position.y - StartY)) - ((b.Position.y - StartY) * (c.Position.x - StartX)));
        float EdgeB_Row = Orientation * (((c.Position.x - StartX) * (a.Position.y - StartY)) - ((c.Position.y - StartY) * (a.Position.x - StartX)));
        const float DepthDX = ((EdgeA_DX * a.InverseDepth) + (EdgeB_DX * b.InverseDepth) - ((EdgeA_DX + EdgeB_DX) * c.InverseDepth)) * InverseArea;
        const float DepthDY = ((EdgeA_DY * a.InverseDepth) + (EdgeB_DY * b.InverseDepth) - ((EdgeA_DY + EdgeB_DY) * c.InverseDepth)) * InverseArea;
        float DepthRow = ((EdgeA_Row * a.InverseDepth) + (EdgeB_Row * b.InverseDepth) + ((std::abs(Area) - EdgeA_Row - EdgeB_Row) * c.InverseDepth)) * InverseArea;
        for (int Y = MinY; Y <= MaxY; ++Y)
        {
            float EdgeA = EdgeA_Row;
            float EdgeB = EdgeB_Row;
            float InverseDepth = DepthRow;
            float * Depth = _DepthBuffer.data() + ((size_t)Y * width) + (size_t)MinX;
            for (int X = MinX; X <= MaxX; ++X)
            {
                const float EdgeC = std::abs(Area) - EdgeA - EdgeB;
                if ((EdgeA >= -0.0001f) && (EdgeB >= -0.0001f) && (EdgeC >= -0.0001f))
                    *Depth = std::max(*Depth, InverseDepth);
                EdgeA += EdgeA_DX;
                EdgeB += EdgeB_DX;
                InverseDepth += DepthDX;
                ++Depth;
            }
            EdgeA_Row += EdgeA_DY;
            EdgeB_Row += EdgeB_DY;
            DepthRow += DepthDY;
        }
    }

    /// <summary>
    /// Clips a wireframe segment against the terrain depth buffer and draws only contiguous visible spans.
    /// </summary>
    void effect_t::DrawDepthClippedLine(ID2D1RenderTarget * renderTarget, const projected_point_t & start, const projected_point_t & end, float strokeWidth, size_t width, size_t height) noexcept
    {
        if (!start.IsVisible || !end.IsVisible)
            return;
        const float DX = end.Position.x - start.Position.x;
        const float DY = end.Position.y - start.Position.y;
        const int StepCount = std::max(1, (int)std::ceil(std::max(std::abs(DX), std::abs(DY)) * 1.5f));
        const float DepthBias = 0.0015f;
        bool SpanOpen = false;
        D2D1_POINT_2F SpanStart{ };
        D2D1_POINT_2F SpanEnd{ };
        float SpanStartDepth = 0.f;
        float SpanEndDepth = 0.f;
        for (int Step = 0; Step <= StepCount; ++Step)
        {
            const float T = (float)Step / (float)StepCount;
            const D2D1_POINT_2F Point = D2D1::Point2F(start.Position.x + DX * T, start.Position.y + DY * T);
            const float InverseDepth = start.InverseDepth + ((end.InverseDepth - start.InverseDepth) * T);
            const int X = (int)std::floor(Point.x);
            const int Y = (int)std::floor(Point.y);
            const bool InViewport = (X >= 0) && (Y >= 0) && ((size_t)X < width) && ((size_t)Y < height);
            const bool IsVisible = InViewport && (InverseDepth + DepthBias >= _DepthBuffer[((size_t)Y * width) + (size_t)X]);
            if (IsVisible)
            {
                if (!SpanOpen)
                {
                    SpanStart = Point;
                    SpanStartDepth = InverseDepth;
                    SpanOpen = true;
                }
                SpanEnd = Point;
                SpanEndDepth = InverseDepth;
            }
            else if (SpanOpen)
            {
                const float AverageDepth = (SpanStartDepth + SpanEndDepth) * 0.5f;
                const float DistanceFade = 0.015f + (0.985f * SmoothStep(0.085f, 0.24f, AverageDepth));
                _LandscapeBrush->SetOpacity(DistanceFade);
                renderTarget->DrawLine(SpanStart, SpanEnd, _LandscapeBrush.Get(), strokeWidth, _RoundStrokeStyle.Get());
                SpanOpen = false;
            }
        }
        if (SpanOpen)
        {
            const float AverageDepth = (SpanStartDepth + SpanEndDepth) * 0.5f;
            const float DistanceFade = 0.015f + (0.985f * SmoothStep(0.085f, 0.24f, AverageDepth));
            _LandscapeBrush->SetOpacity(DistanceFade);
            renderTarget->DrawLine(SpanStart, SpanEnd, _LandscapeBrush.Get(), strokeWidth, _RoundStrokeStyle.Get());
        }
        _LandscapeBrush->SetOpacity(1.f);
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);
        if (FAILED(hr))
            return hr;
        if ((_RenderTarget == renderTarget) && _LandscapeBrush && _GlowBrush && _RoundStrokeStyle)
            return S_OK;
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.55f, 1.00f, 0.68f, 0.90f), _LandscapeBrush.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            return hr;
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.24f, 0.92f, 0.48f, 0.05f), _GlowBrush.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            return hr;
        const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f);
        hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _RoundStrokeStyle.ReleaseAndGetAddressOf());
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
        _LandscapeBrush.Reset();
        _GlowBrush.Reset();
        _RoundStrokeStyle.Reset();
        __super::DeleteDeviceDependentResources();
    }
}

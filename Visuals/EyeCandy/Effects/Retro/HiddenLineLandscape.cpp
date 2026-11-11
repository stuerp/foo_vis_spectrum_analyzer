
/** $VER: HiddenLineLandscape.cpp (2026.11.02) **/

#include <pch.h>

#include "HiddenLineLandscape.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

namespace HiddenLineLandscape
{
    /// <summary>
    ///
    /// </summary>
    effect_t::effect_t()
    {
        StartWorkers();
    }

    /// <summary>
    ///
    /// </summary>
    effect_t::~effect_t()
    {
        StopWorkers();
    }

    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        try
        {
            return std::format(L"Hidden-line landscape, {} quality, {} speed", Qualities[(size_t)_Quality], Speeds[(size_t)_Speed]);
        }
        catch (...)
        {
            return L"Hidden-line landscape";
        }
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

        RenderFrame(renderTarget, D2D1::ColorF(0.60f, 1.00f, 0.72f, 0.20f + (float) (_BeatPulse * 0.22)));
    }

    /// <summary>
    /// Analyzes an audio chunk.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _HasAudio = false;

        _Peak = 0.;
        _RMS  = 0.;

        _BassLevel = 0.;
        _MidLevel  = 0.;

        if (chunk.is_empty())
            return;

        const audio_sample * const Frames = chunk.get_data();

        const auto FrameCount   = chunk.get_sample_count();
        const auto ChannelCount = chunk.get_channel_count();
        const auto SampleRate   = (double) chunk.get_sample_rate();

        // Analyze only the most recent frames of the chunk.
        const auto MaxFrames = std::min<size_t>(FrameCount, 4096);

        if (MaxFrames < 8)
            return;

        double Sum     = 0.;
        double SumBass = 0.;
        double SumMid  = 0.;

        const double SubBassCoefficient = std::exp(-TwoPi * SubBassCutoff / SampleRate);
        const double BassCoefficient    = std::exp(-TwoPi * BassCutoff    / SampleRate);
        const double MidCoefficient     = std::exp(-TwoPi * MidCutoff     / SampleRate);

        const double ChannelScale = 1. / (double) ChannelCount;

        const size_t FrameIndex = FrameCount - MaxFrames;

        const audio_sample * Frame = Frames + (FrameIndex * ChannelCount);
        const audio_sample * const EndFrame = Frame + (MaxFrames * ChannelCount);

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

            _BassState    = (1. - BassCoefficient)    * Mono       + (BassCoefficient    * _BassState);
            _SubBassState = (1. - SubBassCoefficient) * _BassState + (SubBassCoefficient * _SubBassState);
            _MidState     = (1. - MidCoefficient)     * Mono       + (MidCoefficient     * _MidState);

            const double Bass = _BassState - _SubBassState;
            const double Mid  = _MidState  - _BassState;

            SumBass += Bass * Bass;
            SumMid  += Mid * Mid;

            Frame += ChannelCount;
        }

        const double FramesScale = 1. / (double)MaxFrames;

        _RMS = std::sqrt(Sum * FramesScale);

        _BassLevel = std::sqrt(SumBass * FramesScale);
        _MidLevel  = std::sqrt(SumMid  * FramesScale);

        _BassLevel = Clamp(std::log1p(_BassLevel * 36.) / std::log1p(36.));
        _MidLevel  = Clamp(std::log1p(_MidLevel  * 18.) / std::log1p(18.));

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
                case Speed::Slow:
                    return 0.70;

                default:
                case Speed::Normal:
                    return 1.00;

                case Speed::Fast:
                    return 1.85;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        {
            _SmoothedPeak = ExponentialSmoothing(_SmoothedPeak, _Peak,       deltaTime, (_Peak       > _SmoothedPeak) ? 34.0 : 6.5);
            _SmoothedRMS  = ExponentialSmoothing(_SmoothedRMS,  _RMS,        deltaTime, (_RMS        > _SmoothedRMS)  ? 26.0 : 5.5);

            _SmoothedBassLevel = ExponentialSmoothing(_SmoothedBassLevel, _BassLevel, deltaTime, (_BassLevel > _SmoothedBassLevel) ? 42.0 : 12.0);
            _SmoothedMidLevel  = ExponentialSmoothing(_SmoothedMidLevel,  _MidLevel,  deltaTime, (_MidLevel  > _SmoothedMidLevel)  ?  8.0 : 3.0);
            _BeatAverage  = ExponentialSmoothing(_BeatAverage,  _BassLevel, deltaTime, (_BassLevel < _BeatAverage)  ?  5.0 : 0.85);
        }

        const double BassRise          = std::max(0., _BassLevel - _PreviousBass);
        const double BassAboveAverage = std::max(0., _SmoothedBassLevel - _BeatAverage);
        const double AttackStrength   = Clamp((BassAboveAverage * 2.5) + (BassRise * 9.0));

        _PreviousBass      = _BassLevel;
        _BeatCooldown      = std::max(0., _BeatCooldown - deltaTime);
        _PeakSpawnCooldown = std::max(0., _PeakSpawnCooldown - deltaTime);
        _BeatPulse        *= std::exp(-deltaTime * 10.0);

        if (_HasAudio && (_BeatCooldown <= 0.) && (BassRise > 0.008) && (AttackStrength > 0.10))
        {
            _BeatPulse    = std::max(_BeatPulse, AttackStrength);
            _BeatCooldown = 0.085;

            if (_PeakSpawnCooldown <= 0.)
            {
                SpawnPeak(AttackStrength);

                _PeakSpawnCooldown = 0.11;
            }
        }

        // Move only while audio is present. The short release avoids visible start/stop jitter between quiet chunks.
        {
            const double MotionTarget = _HasAudio ? 1. : 0.;

            _AudioMotion = ExponentialSmoothing(_AudioMotion, MotionTarget, deltaTime, _HasAudio ? 14.0 : 4.0);
        }

        const double TravelSpeed = (0.72 + (_SmoothedRMS * 1.10) + (_BeatPulse * 0.52)) * SpeedFactor * _AudioMotion;

        {

            _TerrainTravel += TravelSpeed * deltaTime;
        }

        _CameraSway = ExponentialSmoothing(_CameraSway, std::sin(_AnimationTime * 0.19) * (0.10 + _SmoothedMidLevel * 0.32), deltaTime, 1.8);

        // Follow the actual slope on the camera path. Dense forward samples prevent a narrow peak from falling between probes.
        constexpr double CameraPlane      = 0.10;
        constexpr double BaseCameraHeight = 1.45;
        constexpr double CameraClearance  = 0.38;

        double RequiredCameraHeight = BaseCameraHeight;

        const double CameraX = -_CameraSway;

        for (double Z = CameraPlane; Z <= 7.50; Z += 0.18)
        {
            const double DistanceWeight    = 1. - std::clamp((Z - CameraPlane) / (7.50 - CameraPlane), 0., 1.);
            const double PathHeight        = std::max({ TerrainHeight(CameraX - 0.42, Z), TerrainHeight(CameraX, Z), TerrainHeight(CameraX + 0.42, Z) });
            const double AnticipatedHeight = BaseCameraHeight + ((PathHeight + CameraClearance - BaseCameraHeight) * (0.40 + (0.60 * DistanceWeight)));

            RequiredCameraHeight = std::max(RequiredCameraHeight, AnticipatedHeight);
        }

        // Also inspect each approaching peak analytically. This guarantees enough clearance even before its sampled slope reaches the near field.
        for (const peak_t & Peak : _Peaks)
        {
            const double TrailingExtent = Peak.Width * 3.50;

            if (((Peak.Z + TrailingExtent) < CameraPlane) || (Peak.Z > 8.50))
                continue;

            const double LateralDistance = std::abs(CameraX - Peak.X);
            const double LateralReach    = Peak.Width * 1.35;

            if (LateralDistance > LateralReach)
                continue;

            const double LateralInfluence = 1. - (LateralDistance / LateralReach);
            const double PeakTop          = Peak.Height * Peak.Strength * LateralInfluence;
            const double Approach         = 1. - std::clamp((Peak.Z - CameraPlane) / (8.50 - CameraPlane), 0., 1.);

            // Fade camera support only after the crest has crossed the camera plane and the descending slope is moving past it.
            const double TrailingT         = std::clamp((Peak.Z + TrailingExtent - CameraPlane) / TrailingExtent, 0., 1.);
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
            // Retain the mountain until its crest has passed the camera and its entire descending slope is behind it.
            constexpr double CameraPlane = 0.10;

            const double TrailingExtent = peak.Width * 3.50;

            return (peak.Z < CameraPlane) && ((peak.Z + TrailingExtent) < CameraPlane);
        }), _Peaks.end());
    }

    /// <summary>
    /// Creates a mountain peak near the horizon. Forward terrain motion carries it toward the viewer.
    /// </summary>
    void effect_t::SpawnPeak(double strength) noexcept
    {
        _RandomState = Hash(_RandomState + 0x9E3779B9u);

        peak_t Peak;

        Peak.X        = RandomSigned(_RandomState) * 4.8;
        Peak.Z        = 12.5;
        Peak.Height   = 0.28 + (strength * 0.95) + (Random(_RandomState) * 0.24);
        Peak.Width    = 0.95 + (Random(_RandomState) * 1.20);
        Peak.Strength = 0.68 + (Random(_RandomState) * 0.20);

        // Never evict the oldest mountain merely to admit a new one: it is normally the mountain nearest the camera. Skipping a new transient preserves the complete descending slope of every visible mountain.
        constexpr size_t MaximumPeakCount = 48;

        if (_Peaks.size() >= MaximumPeakCount)
            return;

        try
        {
            _Peaks.push_back(Peak);
        }
        catch (...)
        {
            // A missed transient is preferable to terminating the visualizer on allocation failure.
        }
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

        constexpr double MaximumRadius = 8.;
        constexpr double DepthScale = 1.85;

        for (const peak_t & Peak : _Peaks)
        {
            const double WidthSquared = Peak.Width * Peak.Width;
            const double ZOffset = z - Peak.Z;
            const double NormalizedZSquared = (ZOffset * ZOffset) / (WidthSquared * DepthScale * DepthScale);

            // Reject by depth before evaluating the lateral distance. Most peaks do not overlap a given terrain row.
            if (NormalizedZSquared >= MaximumRadius)
                continue;

            const double XOffset = x - Peak.X;
            const double Radius = ((XOffset * XOffset) / WidthSquared) + NormalizedZSquared;

            if (Radius >= MaximumRadius)
                continue;

            Height += (Peak.Height * Peak.Strength) * std::exp(-Radius * DepthScale);
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

        const double Horizon     = _Height * 0.40;
        const double FocalLength = std::min(_Width, _Height) * 0.92;

        Result.Position.x   = (float) (_CenterX + (((x + _CameraSway) / z) * FocalLength));
        Result.Position.y   = (float) (Horizon + (((1.45 + _CameraElevation - y) / z) * FocalLength));
        Result.InverseDepth = (float) (1. / z);
        Result.IsVisible    = std::isfinite(Result.Position.x) && std::isfinite(Result.Position.y);

        return Result;
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
        const float Pulse = (float) Clamp(_BeatPulse);

        for (size_t i = 0; i < 5; ++i)
        {
            const float Offset = (float) (i * 2);

            _GlowBrush->SetColor(D2D1::ColorF(0.24f, 0.92f, 0.48f, (0.038f - (float) i * 0.006f) + (Pulse * 0.012f)));

            renderTarget->DrawLine(D2D1::Point2F(0.f, Horizon + Offset), D2D1::Point2F(_Width, Horizon + Offset), _GlowBrush.Get(), 1.f);
        }
    }

    /// <summary>
    /// Renders front-to-back terrain rows. A per-pixel upper envelope removes line sections hidden by rows already drawn in front.
    /// </summary>
    void effect_t::RenderLandscape(ID2D1RenderTarget * renderTarget) noexcept
    {
        struct quality_settings_t
        {
            size_t RowCount;
            size_t SamplesPerRow;
            size_t LongitudeCount;
            float DepthScale;
        };

        const quality_settings_t Settings = [&]() noexcept
        {
            switch (_Quality)
            {
                case Quality::Low:
                    return quality_settings_t{ 46, 92, 17, 0.35f };

                default:
                case Quality::Medium:
                    return quality_settings_t{ 76, 156, 25, 0.50f };

                case Quality::High:
                    return quality_settings_t{ 112, 240, 35, 0.75f };
            }
        }();

        if (!std::isfinite(_Width) || !std::isfinite(_Height) || (_Width <= 0.f) || (_Height <= 0.f))
            return;

        const size_t BufferWidth  = std::max<size_t>(2, (size_t)std::ceil(_Width * Settings.DepthScale));
        const size_t BufferHeight = std::max<size_t>(2, (size_t)std::ceil(_Height * Settings.DepthScale));

        if (BufferWidth > (std::numeric_limits<size_t>::max() / BufferHeight))
            return;

        const size_t DepthBufferSize = BufferWidth * BufferHeight;
        const size_t RowCount        = Settings.RowCount;
        const size_t SamplesPerRow   = Settings.SamplesPerRow;

        if (RowCount > (std::numeric_limits<size_t>::max() / SamplesPerRow))
            return;

        const size_t GridSize = RowCount * SamplesPerRow;

        std::vector<double> TerrainRow;

        try
        {
            _DepthBuffer.resize(DepthBufferSize);
            _ProjectedGrid.resize(GridSize);
            TerrainRow.resize(SamplesPerRow);
        }
        catch (...)
        {
            return;
        }

        std::fill(_DepthBuffer.begin(), _DepthBuffer.end(), 0.f);

        // Extend the terrain through the camera plane so a nearby crest remains visible until its descending slope passes.
        constexpr double NearZ  =  0.10;
        constexpr double FarZ   = 13.50;
        constexpr double XRange =  7.40;

        const double XStep        = (XRange * 2.) / (double)(SamplesPerRow - 1);
        const double Horizon      = _Height * 0.40;
        const double FocalLength  = std::min(_Width, _Height) * 0.92;
        const double CameraHeight = 1.45 + _CameraElevation;

        projected_point_t * const Grid = _ProjectedGrid.data();

        for (size_t Row = 0; Row < RowCount; ++Row)
        {
            const double RowT = (double)Row / (double)(RowCount - 1);
            const double Z = NearZ + (RowT * RowT * (FarZ - NearZ));
            const double InverseZ = 1. / Z;
            const double ProjectedScale = FocalLength * InverseZ;

            projected_point_t * const GridRow = Grid + (Row * SamplesPerRow);

            std::fill(TerrainRow.begin(), TerrainRow.end(), 0.);

            if (_HasAudio)
            {
                constexpr double MaximumRadius = 8.;
                constexpr double DepthScale = 1.85;

                // Evaluate peaks row-by-row. Depth rejection happens once per peak, and each surviving peak touches only the contiguous X samples inside its negligible-contribution cutoff.
                for (const peak_t & Peak : _Peaks)
                {
                    const double WidthSquared = Peak.Width * Peak.Width;
                    const double ZOffset = Z - Peak.Z;
                    const double NormalizedZSquared = (ZOffset * ZOffset) / (WidthSquared * DepthScale * DepthScale);

                    if (NormalizedZSquared >= MaximumRadius)
                        continue;

                    const double XReach = Peak.Width * std::sqrt(MaximumRadius - NormalizedZSquared);
                    const double MinimumSampleF = ((Peak.X - XReach) + XRange) / XStep;
                    const double MaximumSampleF = ((Peak.X + XReach) + XRange) / XStep;
                    const size_t FirstSample = (size_t)std::clamp((int)std::ceil(MinimumSampleF), 0, (int)SamplesPerRow - 1);
                    const size_t LastSample = (size_t)std::clamp((int)std::floor(MaximumSampleF), 0, (int)SamplesPerRow - 1);

                    if (FirstSample > LastSample)
                        continue;

                    const double Amplitude = Peak.Height * Peak.Strength;
                    const double InverseWidthSquared = 1. / WidthSquared;

                    double XOffset = (-XRange + ((double)FirstSample * XStep)) - Peak.X;

                    for (size_t Sample = FirstSample; Sample <= LastSample; ++Sample)
                    {
                        const double Radius = (XOffset * XOffset * InverseWidthSquared) + NormalizedZSquared;

                        TerrainRow[Sample] += Amplitude * std::exp(-Radius * DepthScale);
                        XOffset += XStep;
                    }
                }
            }

            for (size_t Sample = 0; Sample < SamplesPerRow; ++Sample)
            {
                const double X = -XRange + ((double)Sample * XStep);
                const double Y = TerrainRow[Sample];

                projected_point_t & Result = GridRow[Sample];

                Result.Position.x = (float)(_CenterX + ((X + _CameraSway) * ProjectedScale));
                Result.Position.y = (float)(Horizon + ((CameraHeight - Y) * ProjectedScale));
                Result.InverseDepth = (float)InverseZ;
                Result.IsVisible = std::isfinite(Result.Position.x) && std::isfinite(Result.Position.y);
            }
        }

        // The terrain surface is rasterized into independent horizontal depth-buffer bands before any wireframe lines are drawn.
        DispatchDepthPrepass(RowCount, SamplesPerRow, BufferWidth, BufferHeight);

        const float Pulse = (float)Clamp(_BeatPulse);
        const float RowStrokeWidth = 0.75f + Pulse * 0.35f;

        _LandscapeBrush->SetColor(D2D1::ColorF(0.55f + Pulse * 0.20f, 1.00f, 0.68f + Pulse * 0.17f, 0.86f));

        // Keep the complete terrain in the depth prepass, but reduce visible line density where perspective compresses adjacent lines. Fixed row bands avoid temporal popping as the terrain and camera move.
        const size_t MiddleDistanceRow = (RowCount * 58) / 100;
        const size_t FarDistanceRow = (RowCount * 78) / 100;

        constexpr size_t MajorContourInterval = 5;

        for (size_t Row = 0; Row < RowCount; ++Row)
        {
            const bool IsMajorContour = (Row % MajorContourInterval) == 0;
            const size_t RowStride = (Row >= FarDistanceRow) ? 4u : ((Row >= MiddleDistanceRow) ? 2u : 1u);

            // Major contours remain visible at every distance; minor contours follow stable power-of-two decimation.
            if (!IsMajorContour && ((Row % RowStride) != 0))
                continue;

            const projected_point_t * const GridRow = Grid + (Row * SamplesPerRow);

            // Keep major contours distinct without letting them dominate the mountain silhouette.
            const float ContourStrokeWidth = IsMajorContour ? (RowStrokeWidth * 1.24f) : (RowStrokeWidth * 0.90f);
            const float ContourOpacity = IsMajorContour ? 0.92f : 0.78f;

            _LandscapeBrush->SetOpacity(ContourOpacity);

            for (size_t Sample = 0; (Sample + 1) < SamplesPerRow; ++Sample)
                DrawDepthClippedLine(renderTarget, GridRow[Sample], GridRow[Sample + 1], ContourStrokeWidth, BufferWidth, BufferHeight);
        }

        const size_t LongitudeStep = std::max<size_t>(1, SamplesPerRow / Settings.LongitudeCount);

        _LandscapeBrush->SetOpacity(0.62f);

        for (size_t Sample = 0; Sample < SamplesPerRow; Sample += LongitudeStep)
        {
            const size_t LongitudeIndex = Sample / LongitudeStep;

            for (size_t Row = 0; (Row + 1) < RowCount; ++Row)
            {
                const size_t LongitudeStride = (Row >= FarDistanceRow) ? 4u : ((Row >= MiddleDistanceRow) ? 2u : 1u);

                // Converging longitude lines use the same stable distance bands as the horizontal contours.
                if ((LongitudeIndex % LongitudeStride) != 0)
                    continue;

                const projected_point_t * const CurrentRow = Grid + (Row * SamplesPerRow);
                const projected_point_t * const NextRow = CurrentRow + SamplesPerRow;

                DrawDepthClippedLine(renderTarget, CurrentRow[Sample], NextRow[Sample], 0.52f, BufferWidth, BufferHeight);
            }
        }

        _LandscapeBrush->SetOpacity(1.f);
    }

    /// <summary>
    /// Dispatches independent horizontal depth-buffer bands and waits for completion.
    /// </summary>
    void effect_t::DispatchDepthPrepass(size_t rowCount, size_t samplesPerRow, size_t width, size_t height) noexcept
    {
        _DepthRowCount = rowCount;
        _DepthSamplesPerRow = samplesPerRow;
        _DepthBufferWidth = width;
        _DepthBufferHeight = height;

        const size_t TriangleCount = (rowCount > 1 && samplesPerRow > 1) ? (rowCount - 1) * (samplesPerRow - 1) * 2 : 0;
        constexpr size_t MinimumTrianglesPerWorker = 2048;
        const size_t UsefulWorkerCount = std::max<size_t>(1, (TriangleCount + MinimumTrianglesPerWorker - 1) / MinimumTrianglesPerWorker);

        _ActiveWorkerCount = std::min({ _Workers.size(), height, UsefulWorkerCount });

        if (_ActiveWorkerCount == 0)
            return;

        {
            const std::lock_guard Lock(_WorkerMutex);

            _PendingWorkers = _ActiveWorkerCount;
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
    }

    /// <summary>
    /// Rasterizes a projected terrain triangle into an inverse-depth buffer. Larger values are closer to the viewer.
    /// </summary>
    void effect_t::RasterizeDepthTriangle(const projected_point_t & a, const projected_point_t & b, const projected_point_t & c, size_t width, size_t height, size_t firstY, size_t lastY) noexcept
    {
        if (!a.IsVisible || !b.IsVisible || !c.IsVisible || (lastY <= firstY))
            return;

        const float ScaleX = (float)width / std::max(1.f, _Width);
        const float ScaleY = (float)height / std::max(1.f, _Height);

        const D2D1_POINT_2F A = D2D1::Point2F(a.Position.x * ScaleX, a.Position.y * ScaleY);
        const D2D1_POINT_2F B = D2D1::Point2F(b.Position.x * ScaleX, b.Position.y * ScaleY);
        const D2D1_POINT_2F C = D2D1::Point2F(c.Position.x * ScaleX, c.Position.y * ScaleY);

        const float MinXF = std::min({ A.x, B.x, C.x });
        const float MaxXF = std::max({ A.x, B.x, C.x });
        const float MinYF = std::min({ A.y, B.y, C.y });
        const float MaxYF = std::max({ A.y, B.y, C.y });

        // Reject off-screen cells before calculating edge equations or touching the depth buffer.
        if ((MaxXF < 0.f) || (MaxYF < (float)firstY) || (MinXF >= (float)width) || (MinYF >= (float)lastY))
            return;

        const float Area = ((B.x - A.x) * (C.y - A.y)) - ((B.y - A.y) * (C.x - A.x));

        if (std::abs(Area) < 1.0e-5f)
            return;

        const int MinX = std::clamp((int)std::floor(MinXF), 0, (int)width - 1);
        const int MaxX = std::clamp((int)std::ceil(MaxXF), 0, (int)width - 1);
        const int MinY = std::clamp((int)std::floor(MinYF), (int)firstY, (int)lastY - 1);
        const int MaxY = std::clamp((int)std::ceil(MaxYF), (int)firstY, (int)lastY - 1);

        if ((MinX > MaxX) || (MinY > MaxY))
            return;

        const float Orientation = (Area < 0.f) ? -1.f : 1.f;
        const float AbsoluteArea = std::abs(Area);
        const float InverseArea = 1.f / AbsoluteArea;

        // Edge equations and inverse depth are advanced incrementally. The inner loop contains additions and comparisons only.
        const float EdgeA_DX = Orientation * (B.y - C.y);
        const float EdgeA_DY = Orientation * (C.x - B.x);
        const float EdgeB_DX = Orientation * (C.y - A.y);
        const float EdgeB_DY = Orientation * (A.x - C.x);
        const float StartX = (float)MinX + 0.5f;
        const float StartY = (float)MinY + 0.5f;

        float EdgeA_Row = Orientation * (((B.x - StartX) * (C.y - StartY)) - ((B.y - StartY) * (C.x - StartX)));
        float EdgeB_Row = Orientation * (((C.x - StartX) * (A.y - StartY)) - ((C.y - StartY) * (A.x - StartX)));

        const float DepthDX = ((EdgeA_DX * a.InverseDepth) + (EdgeB_DX * b.InverseDepth) - ((EdgeA_DX + EdgeB_DX) * c.InverseDepth)) * InverseArea;
        const float DepthDY = ((EdgeA_DY * a.InverseDepth) + (EdgeB_DY * b.InverseDepth) - ((EdgeA_DY + EdgeB_DY) * c.InverseDepth)) * InverseArea;

        float DepthRow = ((EdgeA_Row * a.InverseDepth) + (EdgeB_Row * b.InverseDepth) + ((AbsoluteArea - EdgeA_Row - EdgeB_Row) * c.InverseDepth)) * InverseArea;

        for (int Y = MinY; Y <= MaxY; ++Y)
        {
            float EdgeA = EdgeA_Row;
            float EdgeB = EdgeB_Row;
            float InverseDepth = DepthRow;

            float * Depth = _DepthBuffer.data() + ((size_t)Y * width) + (size_t)MinX;

            for (int X = MinX; X <= MaxX; ++X)
            {
                const float EdgeC = AbsoluteArea - EdgeA - EdgeB;

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

        float T0 = 0.f;
        float T1 = 1.f;

        const float DX = end.Position.x - start.Position.x;
        const float DY = end.Position.y - start.Position.y;

        const auto Clip = [&T0, &T1](float p, float q) noexcept
        {
            if (std::abs(p) < 1.0e-8f)
                return q >= 0.f;

            const float R = q / p;

            if (p < 0.f)
            {
                if (R > T1)
                    return false;

                T0 = std::max(T0, R);
            }
            else
            {
                if (R < T0)
                    return false;

                T1 = std::min(T1, R);
            }

            return true;
        };

        if (!Clip(-DX, start.Position.x) || !Clip(DX, _Width - start.Position.x) || !Clip(-DY, start.Position.y) || !Clip(DY, _Height - start.Position.y) || (T1 < T0))
        {
            return;
        }

        const float ClippedStartX = start.Position.x + (DX * T0);
        const float ClippedStartY = start.Position.y + (DY * T0);
        const float ClippedEndX = start.Position.x + (DX * T1);
        const float ClippedEndY = start.Position.y + (DY * T1);
        const float ClippedStartDepth = start.InverseDepth + ((end.InverseDepth - start.InverseDepth) * T0);
        const float ClippedEndDepth = start.InverseDepth + ((end.InverseDepth - start.InverseDepth) * T1);
        const float ClippedDX = ClippedEndX - ClippedStartX;
        const float ClippedDY = ClippedEndY - ClippedStartY;
        const float ScaleX = (float)width / std::max(1.f, _Width);
        const float ScaleY = (float)height / std::max(1.f, _Height);
        const int StepCount = std::max(1, (int)std::ceil(std::max(std::abs(ClippedDX) * ScaleX, std::abs(ClippedDY) * ScaleY)));
        const float InverseSteps = 1.f / (float)StepCount;
        const float StepX = ClippedDX * InverseSteps;
        const float StepY = ClippedDY * InverseSteps;
        const float StepDepth = (ClippedEndDepth - ClippedStartDepth) * InverseSteps;
        const float StepDepthX = StepX * ScaleX;
        const float StepDepthY = StepY * ScaleY;
        const int MaximumDepthX = (int)width - 1;
        const int MaximumDepthY = (int)height - 1;
        const float DepthBias = 0.0015f;
        bool SpanOpen = false;
        D2D1_POINT_2F SpanStart{ };
        D2D1_POINT_2F SpanEnd{ };
        float SpanStartDepth = 0.f;
        float SpanEndDepth = 0.f;
        float X = ClippedStartX;
        float Y = ClippedStartY;
        float DepthSampleX = ClippedStartX * ScaleX;
        float DepthSampleY = ClippedStartY * ScaleY;
        float InverseDepth = ClippedStartDepth;

        // Capture invariant drawing state once per source line. A line may produce several visible spans.
        const float FamilyOpacity = _LandscapeBrush->GetOpacity();
        const D2D1_COLOR_F ContourColor = _LandscapeBrush->GetColor();
        const D2D1_SIZE_F RenderSize = renderTarget->GetSize();
        const D2D1_SIZE_U PixelSize = renderTarget->GetPixelSize();
        const float PixelWidth = (RenderSize.width > 0.f) ? ((float)PixelSize.width / RenderSize.width) : 1.f;
        const float PixelHeight = (RenderSize.height > 0.f) ? ((float)PixelSize.height / RenderSize.height) : 1.f;
        const float SafePixelWidth = std::max(PixelWidth, 0.0001f);
        const float SafePixelHeight = std::max(PixelHeight, 0.0001f);
        const bool IsMajorContour = FamilyOpacity >= 0.85f;

        const auto DrawSpan = [&](const D2D1_POINT_2F & spanStart, const D2D1_POINT_2F & spanEnd, float startDepth, float endDepth) noexcept
        {
            const float AverageDepth = (startDepth + endDepth) * 0.5f;
            const float DistanceFade = 0.015f + (0.985f * SmoothStep(0.085f, 0.24f, AverageDepth));

            // Preserve near-camera geometry for depth testing, but dissolve visible lines as they pass beneath the viewer.
            constexpr float NearFadeStart = 3.20f;
            constexpr float NearFadeEnd = 8.60f;

            const float NearFade = 1.f - SmoothStep(NearFadeStart, NearFadeEnd, AverageDepth);

            constexpr float OpacityLevels = 15.f;

            const float QuantizedOpacity = std::round((DistanceFade * NearFade * FamilyOpacity) * OpacityLevels) / OpacityLevels;

            // Stabilize nearly axis-aligned spans on physical pixel centers. Small camera movements then keep
            // antialias coverage consistent, while genuinely sloped mountain geometry remains untouched.
            D2D1_POINT_2F StableStart = spanStart;
            D2D1_POINT_2F StableEnd = spanEnd;

            const float SpanDX = spanEnd.x - spanStart.x;
            const float SpanDY = spanEnd.y - spanStart.y;

            if (std::abs(SpanDY) < (0.65f / SafePixelHeight))
            {
                const float DeviceY = (((spanStart.y + spanEnd.y) * 0.5f) * SafePixelHeight);
                const float StableY = (std::floor(DeviceY) + 0.5f) / SafePixelHeight;

                StableStart.y = StableY;
                StableEnd.y = StableY;
            }
            else
            if (std::abs(SpanDX) < (0.65f / SafePixelWidth))
            {
                const float DeviceX = (((spanStart.x + spanEnd.x) * 0.5f) * SafePixelWidth);
                const float StableX = (std::floor(DeviceX) + 0.5f) / SafePixelWidth;

                StableStart.x = StableX;
                StableEnd.x   = StableX;
            }

            // Add a restrained halo only to major contours. Suppress it near the horizon, where broad lines
            // would reinforce interference patterns, and let the existing near-camera dissolve control its exit.
            const bool IsOutsideHorizonCompression = AverageDepth >= 0.11f;

            if (IsMajorContour && IsOutsideHorizonCompression && (QuantizedOpacity > 0.f))
            {
                const float GlowOpacity = QuantizedOpacity * 0.075f;

                _GlowBrush->SetColor(D2D1::ColorF(ContourColor.r, ContourColor.g, ContourColor.b, GlowOpacity));

                renderTarget->DrawLine(StableStart, StableEnd, _GlowBrush.Get(), strokeWidth * 2.25f, _RoundStrokeStyle.Get());
            }

            if (QuantizedOpacity > 0.f)
            {
                _LandscapeBrush->SetOpacity(QuantizedOpacity);

                renderTarget->DrawLine(StableStart, StableEnd, _LandscapeBrush.Get(), strokeWidth, _RoundStrokeStyle.Get());
            }
        };

        const float * const DepthBuffer = _DepthBuffer.data();

        for (int Step = 0; Step <= StepCount; ++Step)
        {
            // Clipping keeps samples inside the viewport. Manual saturation handles the inclusive far edge and
            // floating-point drift without floor(), two multiplications, and two generic clamp calls per sample.
            const int UnclampedDepthX = (int)DepthSampleX;
            const int UnclampedDepthY = (int)DepthSampleY;
            const int DepthX = (UnclampedDepthX < 0) ? 0 : ((UnclampedDepthX > MaximumDepthX) ? MaximumDepthX : UnclampedDepthX);
            const int DepthY = (UnclampedDepthY < 0) ? 0 : ((UnclampedDepthY > MaximumDepthY) ? MaximumDepthY : UnclampedDepthY);
            const size_t DepthIndex = ((size_t)DepthY * width) + (size_t)DepthX;
            const bool IsVisible = InverseDepth + DepthBias >= DepthBuffer[DepthIndex];

            if (IsVisible)
            {
                if (!SpanOpen)
                {
                    SpanStart = D2D1::Point2F(X, Y);
                    SpanStartDepth = InverseDepth;
                    SpanOpen = true;
                }

                SpanEnd = D2D1::Point2F(X, Y);
                SpanEndDepth = InverseDepth;
            }
            else
            if (SpanOpen)
            {
                DrawSpan(SpanStart, SpanEnd, SpanStartDepth, SpanEndDepth);
                SpanOpen = false;
            }

            X += StepX;
            Y += StepY;

            DepthSampleX += StepDepthX;
            DepthSampleY += StepDepthY;
            InverseDepth += StepDepth;
        }

        if (SpanOpen)
            DrawSpan(SpanStart, SpanEnd, SpanStartDepth, SpanEndDepth);

        // Preserve the family opacity selected by RenderLandscape for subsequent source-line segments.
        _LandscapeBrush->SetOpacity(FamilyOpacity);
    }

    /// <summary>
    /// Start the worker threads.
    /// </summary>
    void effect_t::StartWorkers()
    {
        if (!_Workers.empty())
            return;

        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, _MaxWorkers);

        // A restarted pool begins at a known generation. This prevents new workers from consuming a stale dispatch.
        {
            const std::lock_guard Lock(_WorkerMutex);

            _IsStopping = false;
            _Generation = 0;

            _PendingWorkers = 0;
            _ActiveWorkerCount = 0;
        }

        try
        {
            _Workers.reserve(WorkerCount);

            for (size_t i = 0; i < WorkerCount; ++i)
            {
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

        for (auto & Worker : _Workers)
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
    void effect_t::Worker(size_t workerIndex) noexcept
    {
        size_t Generation = 0;

        {
            const std::lock_guard Lock(_WorkerMutex);

            Generation = _Generation;
        }

        for (;;)
        {
            {
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
                });

                if (_IsStopping)
                    return;

                Generation = _Generation;
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
    /// Rasterizes all terrain triangles clipped to one non-overlapping horizontal band.
    /// </summary>
    void effect_t::RasterizeDepthBand(size_t workerIndex) noexcept
    {
        const size_t FirstY = (_DepthBufferHeight * workerIndex) / _ActiveWorkerCount;
        const size_t LastY = (_DepthBufferHeight * (workerIndex + 1)) / _ActiveWorkerCount;

        const float DepthScaleY = (float)_DepthBufferHeight / std::max(1.f, _Height);
        const float FirstScreenY = (float)FirstY / DepthScaleY;
        const float LastScreenY = (float)LastY / DepthScaleY;

        const projected_point_t * const Grid = _ProjectedGrid.data();
        const size_t SampleCount = _DepthSamplesPerRow;

        for (size_t Row = 0; (Row + 1) < _DepthRowCount; ++Row)
        {
            const projected_point_t * const CurrentRow = Grid + (Row * SampleCount);
            const projected_point_t * const NextRow = CurrentRow + SampleCount;

            float RowMinY = std::numeric_limits<float>::max();
            float RowMaxY = std::numeric_limits<float>::lowest();

            // Reject complete row strips before visiting individual cells. With perspective projection, most
            // row strips overlap only one or two worker bands, eliminating repeated cell traversal elsewhere.
            for (size_t Sample = 0; Sample < SampleCount; ++Sample)
            {
                const projected_point_t & A = CurrentRow[Sample];
                const projected_point_t & C = NextRow[Sample];

                if (A.IsVisible)
                {
                    RowMinY = std::min(RowMinY, A.Position.y);
                    RowMaxY = std::max(RowMaxY, A.Position.y);
                }

                if (C.IsVisible)
                {
                    RowMinY = std::min(RowMinY, C.Position.y);
                    RowMaxY = std::max(RowMaxY, C.Position.y);
                }
            }

            if ((RowMaxY < FirstScreenY) || (RowMinY >= LastScreenY))
                continue;

            const projected_point_t * A = &CurrentRow[0];
            const projected_point_t * C = &NextRow[0];

            for (size_t Sample = 0; (Sample + 1) < SampleCount; ++Sample)
            {
                const projected_point_t * const B = A + 1;
                const projected_point_t * const D = C + 1;

                if ((!A->IsVisible && !B->IsVisible && !C->IsVisible && !D->IsVisible))
                {
                    A = B;
                    C = D;
                    continue;
                }

                const float LeftMinY = std::min(A->Position.y, C->Position.y);
                const float LeftMaxY = std::max(A->Position.y, C->Position.y);
                const float RightMinY = std::min(B->Position.y, D->Position.y);
                const float RightMaxY = std::max(B->Position.y, D->Position.y);
                const float CellMinY = std::min(LeftMinY, RightMinY);
                const float CellMaxY = std::max(LeftMaxY, RightMaxY);

                if ((CellMaxY >= FirstScreenY) && (CellMinY < LastScreenY))
                {
                    // Test each triangle against the band separately. Mountain slopes frequently split a cell
                    // across a band boundary, so this avoids triangle setup for the non-overlapping half.
                    const float FirstTriangleMinY = std::min(std::min(A->Position.y, B->Position.y), C->Position.y);
                    const float FirstTriangleMaxY = std::max(std::max(A->Position.y, B->Position.y), C->Position.y);

                    if ((FirstTriangleMaxY >= FirstScreenY) && (FirstTriangleMinY < LastScreenY))
                        RasterizeDepthTriangle(*A, *B, *C, _DepthBufferWidth, _DepthBufferHeight, FirstY, LastY);

                    const float SecondTriangleMinY = std::min(std::min(B->Position.y, C->Position.y), D->Position.y);
                    const float SecondTriangleMaxY = std::max(std::max(B->Position.y, C->Position.y), D->Position.y);

                    if ((SecondTriangleMaxY >= FirstScreenY) && (SecondTriangleMinY < LastScreenY))
                        RasterizeDepthTriangle(*B, *D, *C, _DepthBufferWidth, _DepthBufferHeight, FirstY, LastY);
                }

                A = B;
                C = D;
            }
        }
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

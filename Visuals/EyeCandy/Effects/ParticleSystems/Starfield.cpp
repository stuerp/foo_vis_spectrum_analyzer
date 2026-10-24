
/** $VER: Starfield.cpp (2026.10.22) **/

#include <pch.h>

#include "Starfield.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>

namespace Starfield
{
    using EyeCandy::ThreeD;

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
        return std::format(L"Starfield, {} quality, {} speed", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
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

        Projection.Width       = (double) _Width;
        Projection.Height      = (double) _Height;
        Projection.AspectRatio = (_Height > 0.f) ? (double) _Width / (double) _Height : 1.;

        AnalyzeAudio(chunk);
        Update(deltaTime);

        {
            renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            {
                const double Yaw   = _FlightYaw   * HorizontalVanishingPointRange;
                const double Pitch = _FlightPitch * HVerticalVanishingPointRange;

                const double HorizontalDrift = std::sin(_AnimationTime * 0.31) * 0.01;
                const double VerticalDrift   = std::cos(_AnimationTime * 0.23) * 0.01;

                // Steering uses an asymmetric projection instead of tilting the flight axis. This keeps star depth
                // aligned with movement, so stars continue through the near plane at every steering angle.
                Projection.LensOffsetX = (HorizontalDrift - Yaw) * 2.;
                Projection.LensOffsetY = (Pitch - VerticalDrift) * 2.;

                Camera.Target = { 0., 0., 1. };

                // Roll the camera basis rather than the completed 2D image. Projection and clipping now use the
                // same banked coordinate frame, avoiding the loss of fly-past motion at large roll angles.
                const double Roll = _CameraRoll * Pi / 180.;

                Camera.Up = { std::sin(Roll), std::cos(Roll), 0. };

                const auto PreparedProjection = ThreeD::Prepare();
                const auto ProjectedCenter    = ThreeD::Project({ 0., 0., Projection.FarPlane * 0.5 }, PreparedProjection);

                _Center = ProjectedCenter.IsProjectable ? ProjectedCenter.Position : D2D1::Point2F(_CenterX, _CenterY);
            }

            RenderBackground(renderTarget);
            RenderStars(renderTarget);
            RenderFlash(renderTarget);
        }

        RenderScanlines(renderTarget);

        // Thin border reinforces the appearance of a retro display and hides clipped trail ends at the viewport edge.
        _FrameBrush->SetColor(HSVToColor(CurrentThemeHue(), 0.58f, 0.72f, 0.22f + (float) _BeatPulse * 0.18f));

        renderTarget->DrawRectangle(D2D1::RectF(0.5f, 0.5f, _Width - 0.5f, _Height - 0.5f), _FrameBrush.Get(), 1.f);
    }

    /// <summary>
    /// Analyzes an audio chunk.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _Peak       = 0.;
        _RMS        = 0.;

        _BassEnergy = 0.;
        _MidEnergy  = 0.;

        _HasAudio = false;

        if (chunk.is_empty())
            return;

        const size_t FrameCount   = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();
        const size_t SampleRate   = chunk.get_sample_rate();

        // Analyze only the most recent 4096 sample frames.
        const size_t MaxFrames = std::min<size_t>(FrameCount, 4096);
        const size_t FrameIndex = FrameCount - MaxFrames;

        if (MaxFrames < 8)
            return;

        const audio_sample * Frame = chunk.get_data() + (FrameIndex * ChannelCount);
        const audio_sample * const End = Frame + (MaxFrames * ChannelCount);

        // Initialize the energy accumulators.
        double Sum     = 0.;
        double SumBass = 0.;
        double SumMid  = 0.;

        // Two cascaded one-pole filters create a lightweight low-frequency detector. The first follows content below approximately 180 Hz.
        // Subtracting the slower 35 Hz follower rejects DC and subsonic content.
        {
            // Configure the coefficients. Generic formula: coefficient = e^(-2π × cutoff / sample rate).
            // A coefficient close to 1 means that the filter changes slowly and remembers more of its previous state.
            // _BassState and _DCState track the configured frequencies.
            const double DCCoefficient   = std::exp(-TwoPi *   35.0 / (double) SampleRate);
            const double BassCoefficient = std::exp(-TwoPi *  180.0 / (double) SampleRate);
            const double MidCoefficient  = std::exp(-TwoPi * 2000.0 / (double) SampleRate);

            const double ChannelScale = 1. / (double) ChannelCount;

            while (Frame < End)
            {
                double Mono = 0.;

                const audio_sample * Channel = Frame;
                const audio_sample * const ChannelEnd = Frame + ChannelCount;

                while (Channel < ChannelEnd)
                    Mono += *Channel++;

                const double Sample = ClampAudio(Mono * ChannelScale);

                _Peak = std::max(_Peak, std::abs(Sample));

                Sum += Sample * Sample;

                _BassState = (1.0 - BassCoefficient) * Sample + BassCoefficient * _BassState;
                _DCState   = (1.0 - DCCoefficient) * _BassState + DCCoefficient * _DCState;

                const double Bass = _BassState - _DCState;

                SumBass += Bass * Bass;

                _MidState = (1.0 - MidCoefficient) * Sample + MidCoefficient * _MidState;

                const double Mid = _MidState - _BassState;

                SumMid += Mid * Mid;

                Frame += ChannelCount;
            }
        }

        const double CountScale = 1. / (double) MaxFrames;

        _RMS        = std::sqrt(Sum     * CountScale);
        _BassEnergy = std::sqrt(SumBass * CountScale);
        _MidEnergy  = std::sqrt(SumMid  * CountScale);

        // Compress the bass and mid energy into ranges useful for visualization.
        _BassEnergy = Clamp(std::log1p(_BassEnergy * 36.) / std::log1p(36.));
        _MidEnergy  = Clamp(std::log1p(_MidEnergy  * 18.) / std::log1p(18.));

        _HasAudio = (_Peak > 1.0e-5) || (_RMS > 1.0e-6);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime)
    {
        if (_Stars.empty())
            Reset();

        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case EyeCandy::Speed::Slow:
                    return 0.82;

                case EyeCandy::Speed::Fast:
                    return 2.45;

                case EyeCandy::Speed::Normal:
                default:
                    return 1.35;
            }
        }();

        _AnimationTime  += deltaTime * SpeedFactor;
        _TransitionTime += deltaTime;   // Cycle through galaxy-inspired color themes every five seconds.

        while (_TransitionTime >= TransitionInterval)
        {
            _TransitionTime -= TransitionInterval;
            _ThemeIndex = (_ThemeIndex + 1) % ThemeCount;
        }

        // Apply temporal smoothing. Choose a different response rate based on a rising or falling signal.
        {
            {
                // Rises quickly when a larger peak arrives and falls more slowly after the peak disappears.
                constexpr double AttackRate  = 34.0;
                constexpr double ReleaseRate =  6.5;

                _SmoothedPeak = ExponentialSmoothing(_SmoothedPeak, _Peak, deltaTime, (_Peak > _SmoothedPeak) ? AttackRate : ReleaseRate);
            }

            {
                // RMS rises slightly more slowly and falls slightly more slowly compared to Peak. It represents sustained loudness instead of sharp transients.
                constexpr double AttackRate  = 26.0;
                constexpr double ReleaseRate =  5.5;

                _SmoothedRMS  = ExponentialSmoothing(_SmoothedRMS,  _RMS, deltaTime, (_RMS > _SmoothedRMS) ? AttackRate : ReleaseRate);
            }

            {
                // High attack rate to detect drums which have a very short leading edge and falls faster than the other smoothed measurements. Makes consecutive bass hits distinguishable.
                constexpr double AttackRate  = 42.0; 
                constexpr double ReleaseRate = 12.0;

                _SmoothedBass = ExponentialSmoothing(_SmoothedBass, _BassEnergy, deltaTime, (_BassEnergy > _SmoothedBass) ? AttackRate : ReleaseRate);
            }

            {
                // Mid frequencies use a gentler response so vocals and instruments sway the camera without introducing abrupt rotation.
                constexpr double AttackRate  = 6.0;
                constexpr double ReleaseRate = 2.5;

                _SmoothedMid = ExponentialSmoothing(_SmoothedMid, _MidEnergy, deltaTime, (_MidEnergy > _SmoothedMid) ? AttackRate : ReleaseRate);
            }

            {
                // The baseline falls moderately quickly but rises very slowly. That asymmetry is intentional. When a kick arrives,
                // _BassEnergy rises rapidly. If _BeatAverage rose at the same speed, the difference between them would disappear.
                // Prevents continuously loud bass from being interpreted as a new beat forever.
                constexpr double AttackRate  = 5.00; 
                constexpr double ReleaseRate = 0.85;

                _BeatAverage = ExponentialSmoothing(_BeatAverage, _BassEnergy, deltaTime, (_BassEnergy < _BeatAverage) ? AttackRate : ReleaseRate);
            }
        }

        {
            const double BassRise         = std::max(0., _BassEnergy - _PreviousBass);  // Detects sudden changes.
            const double BassAboveAverage = std::max(0., _SmoothedBass - _BeatAverage); // Detects relative strengt.

            // BassRise receives much more weight because the desired effect is a sudden acceleration on the leading edge of a bass hit.
            const double AttackStrength = Clamp((BassAboveAverage * 2.5) + (BassRise * 9.0));

            _PreviousBass = _BassEnergy;

            {
                _BeatCooldown = std::max(0., _BeatCooldown - deltaTime);
                _BeatPulse *= std::exp(-deltaTime * 13.0);

                if (_HasAudio && (_BeatCooldown <= 0.0) && (BassRise > 0.008) && (AttackStrength > 0.10))
                {
                    _BeatPulse = std::max(_BeatPulse, AttackStrength);
                    _BeatCooldown = 0.085;
                }
            }
        }

        // Rotate the starfield with the mid frequencies. Strong mid-frequency rises reverse the direction more frequently while a brief counter impulse makes each reversal more dramatic.
        {
            constexpr double MaximumRotationSpeed = 14.0;

            const double MidRise = std::max(0., _MidEnergy - _PreviousMid);
            const double MidSway = (std::sin(_AnimationTime * 0.57) * 0.64) + (std::sin(_AnimationTime * 0.193) * 0.36);

            _MidDirectionCooldown = std::max(0., _MidDirectionCooldown - deltaTime);

            if (_HasAudio && (_MidDirectionCooldown <= 0.) && (MidRise > 0.025) && (_SmoothedMid > 0.12))
            {
                _CameraRollDirection = -_CameraRollDirection;
                _CameraRollVelocity = _CameraRollDirection * std::min(MaximumRotationSpeed, 7.5 + (_SmoothedMid * 6.5));
                _MidDirectionCooldown = 0.55 + ((1. - _SmoothedMid) * 0.85);
            }

            const double TargetVelocity = std::clamp(_CameraRollDirection * (2.4 + (_SmoothedMid * 10.5)) + (MidSway * _SmoothedMid * 3.6), -MaximumRotationSpeed, MaximumRotationSpeed);

            _CameraRollVelocity = ExponentialSmoothing(_CameraRollVelocity, TargetVelocity, deltaTime, 4.8);
            _CameraRollVelocity = std::clamp(_CameraRollVelocity, -MaximumRotationSpeed, MaximumRotationSpeed);
            _CameraRoll += _CameraRollVelocity * deltaTime;

            if (_CameraRoll > 180.)
                _CameraRoll -= 360.;
            else
            if (_CameraRoll < -180.)
                _CameraRoll += 360.;

            _PreviousMid = _MidEnergy;
        }

        // Simulate independent spaceship steering. Slow layered oscillators create wide left and right turns while the smaller pitch range moves the nose gently up and down.
        {
            const double Intensity = [&]() noexcept
            {
                switch (_FlightMotionIntensity)
                {
                    case FlightMotionIntensity::Subtle:
                        return 0.60;

                    case FlightMotionIntensity::Strong:
                        return 1.45;

                    case FlightMotionIntensity::Normal:
                    default:
                        return 1.00;
                }
            }();

            const bool FlightActive = _FlightMotionEnabled && _HasAudio;
            const double FlightScale = FlightActive ? Intensity : 0.;

            const double TargetYaw   = FlightScale * ((std::sin(_AnimationTime * 0.075)       * 0.74) + (std::sin(_AnimationTime * 0.029 + 1.7) * 0.26));
            const double TargetPitch = FlightScale * ((std::sin(_AnimationTime * 0.083 + 0.8) * 0.70) + (std::sin(_AnimationTime * 0.031 + 2.4) * 0.30));
            const double VelocityRate = FlightActive ? 0.80 : 24.0;
            const double PositionRate = FlightActive ? 0.62 : 18.0;

            _FlightYawVelocity   = ExponentialSmoothing(_FlightYawVelocity,   TargetYaw,   deltaTime, VelocityRate);
            _FlightPitchVelocity = ExponentialSmoothing(_FlightPitchVelocity, TargetPitch, deltaTime, VelocityRate);

            _FlightYaw   = ExponentialSmoothing(_FlightYaw,   _FlightYawVelocity,   deltaTime, PositionRate);
            _FlightPitch = ExponentialSmoothing(_FlightPitch, _FlightPitchVelocity, deltaTime, PositionRate);

            if (!FlightActive && (std::abs(_FlightYaw) < 1.0e-3) && (std::abs(_FlightPitch) < 1.0e-3))
            {
                _FlightYaw = 0.;
                _FlightPitch = 0.;
                _FlightYawVelocity = 0.;
                _FlightPitchVelocity = 0.;
            }
        }

        // Keep normal movement restrained so the short bass impulse remains clearly visible as acceleration.
        {
            const double BaseSpeed    = 0.18 + (_SmoothedRMS * 0.72) + (_SmoothedBass * 0.22) + (_SmoothedPeak * 0.12);
            const double BeatSpeed    = _BeatPulse * 3.20;
            const double ForwardSpeed = (BaseSpeed + BeatSpeed) * SpeedFactor;

            _TravelDistance += ForwardSpeed * deltaTime;

            GenerateWork(deltaTime, ForwardSpeed);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        DeleteDeviceDependentResources();

        Projection.Width       = (double) _Width;
        Projection.Height      = (double) _Height;
        Projection.AspectRatio = (_Height > 0.f) ? (double) _Width / (double) _Height : 1.;

        World  = {};
        Camera = {};
        Object = {};

        Projection.LensOffsetX = 0.;
        Projection.LensOffsetY = 0.;

        _BassState = 0.;
        _DCState   = 0.;
        _MidState  = 0.;

        _MidEnergy = 0.;

        _SmoothedMid = 0.;
        _PreviousMid = 0.;

        _MidDirectionCooldown = 0.;

        _CameraRollVelocity  = 0.;
        _CameraRollDirection = 1.;

        _FlightYaw           = 0.;
        _FlightPitch         = 0.;
        _FlightYawVelocity   = 0.;
        _FlightPitchVelocity = 0.;

        _PreviousBass = 0.;
        _BeatAverage  = 0.;
        _BeatPulse    = 0.;
        _BeatCooldown = 0.;

        _Stars.clear();

        const size_t Count = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low:
                    return size_t{ 450 };

                case EyeCandy::Quality::High:
                    return size_t{ 1800 };

                case EyeCandy::Quality::Medium:
                default:
                    return size_t{ 950 };
            }
        }();

        _Stars.resize(Count);

        for (size_t i = 0; i < Count; ++i)
            RespawnStar(_Stars[i], i, true);
    }

    /// <summary>
    /// Generates work for the worker threads.
    /// </summary>
    void effect_t::GenerateWork(double deltaTime, double forwardSpeed) noexcept
    {
        if (_Stars.empty() || _Workers.empty())
            return;

        {
            const std::lock_guard Lock(_WorkerMutex);

            _StarDeltaTime = deltaTime;
            _ForwardSpeed  = forwardSpeed;

            _NextStar.store(0, std::memory_order_relaxed);

            _PendingWorkers = _Workers.size();
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
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StartWorkers()
    {
        const auto WorkerCount = (size_t) std::max(1u, std::thread::hardware_concurrency());

        _Workers.reserve(WorkerCount);

        try
        {
            for (size_t i = 0; i < WorkerCount; ++i)
            {
                // Pass a closure containing an ordinary object pointer instead of a pointer-to-member function. MSVC stores std::thread arguments in an
                // internal tuple; a member-function pointer for this multiply inherited class triggers C4371 in std::_Tuple_val on newer compiler versions.
                _Workers.emplace_back([this]() noexcept
                {
                    Worker();
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

        for (auto & worker : _Workers)
        {
            if (worker.joinable())
                worker.join();
        }

        _Workers.clear();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Worker() noexcept
    {
        size_t Generation = 0;

        for (;;)
        {
            double DeltaTime;
            double ForwardSpeed;

            {
                std::unique_lock lock(_WorkerMutex);

                _WorkerCondition.wait(lock, [this, &Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
                });

                if (_IsStopping)
                    return;

                DeltaTime    = _StarDeltaTime;
                ForwardSpeed = _ForwardSpeed;

                Generation = _Generation;
            }

            for (;;)
            {
                const size_t StarIndex = _NextStar.fetch_add(1, std::memory_order_relaxed);

                if (StarIndex >= _Stars.size())
                    break;

                UpdateStar(StarIndex, DeltaTime, ForwardSpeed);
            }
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
    void effect_t::UpdateStar(size_t index, double deltaTime, double forwardSpeed) noexcept
    {
        star_t & Star = _Stars[index];

        Star.PreviousZ = Star.Z;
        Star.Z        -= forwardSpeed * Star.Speed * deltaTime;

        if (Star.Z <= Projection.NearPlane)
            RespawnStar(Star, index, false);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RespawnStar(star_t & star, size_t index, bool randomDepth) noexcept
    {
        uint32_t State = Hash((uint32_t) index * 0x9E3779B9u ^ star.Seed ^ (uint32_t)(_TravelDistance * 100'000.0f));

        star.Seed = State;

        // Polar distribution avoids visible square corners while retaining enough offscreen stars to produce full-width trails.
        const double Angle  = Random(State) * TwoPi;
        const double Radius = std::sqrt(Random(State)) * 1.36f;

        star.X = std::cos(Angle) * Radius;
        star.Y = std::sin(Angle) * Radius;

        // Slightly flatten the field to make the scene feel widescreen.
        star.Y *= 0.82;
        star.Z  = randomDepth ? Lerp(Projection.NearPlane + 0.015, Projection.FarPlane, (double) Random(State)) : Projection.FarPlane;

        star.PreviousZ = star.Z;

        star.Speed      = Lerp(0.72, 1.35, (double) Random(State));
        star.Size       = Lerp(0.55, 1.65, (double) Random(State));
        star.Brightness = Lerp(0.34, 1.0,  (double) Random(State));
        star.HueOffset  = RandomSigned(State);
    }

    /// <summary>
    /// 
    /// </summary>
    float effect_t::CurrentThemeHue() const noexcept
    {
        const size_t PreviousTheme = (_ThemeIndex + ThemeCount - 1) % ThemeCount;
        const float StartHue = ThemeHues[PreviousTheme];
        const float EndHue = ThemeHues[_ThemeIndex];

        float Difference = EndHue - StartHue;

        if (Difference > 0.5f)
            Difference -= 1.f;
        else
        if (Difference < -0.5f)
            Difference += 1.f;

        const float Blend = SmoothStep(0.f, (float) ThemeTransitionTime, (float) std::min(_TransitionTime, ThemeTransitionTime));

        return Fraction(StartHue + Difference * Blend);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderBackground(ID2D1RenderTarget * renderTarget) noexcept
    {
        renderTarget->Clear(D2D1::ColorF( 0.0015f, 0.003f, 0.009f, 1.f));

        _BackgroundBrush->SetCenter(_Center);
        _BackgroundBrush->SetGradientOriginOffset(D2D1::Point2F((float) (std::sin(_AnimationTime * 0.17) * _Width * 0.01), (float) (std::cos(_AnimationTime * 0.13) * _Height * 0.01)));

        _BackgroundBrush->SetRadiusX(_Width  * 0.72f);
        _BackgroundBrush->SetRadiusY(_Height * 0.72f);

        _BackgroundBrush->SetOpacity((float) Clamp(0.72 + (_SmoothedRMS * 0.4) + (_BeatPulse * 0.18)));

        renderTarget->FillRectangle(_Rectangle, _BackgroundBrush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderStars(ID2D1RenderTarget * renderTarget)
    {
        std::array<ComPtr<ID2D1PathGeometry>, TrailLayerCount> Trails;

        GenerateStarTrails(Trails);

        const float Pulse = (float) Clamp(_BeatPulse);

        // Draw each trail batch multiple times. Wide low-opacity strokes create the glow; the final narrow stroke supplies the bright core.
        {
            const float ThemeHue = CurrentThemeHue();

            const D2D1_COLOR_F TrailColors[TrailLayerCount] =
            {
                HSVToColor(Fraction(ThemeHue - 0.035f), 0.78f, 0.92f, 0.13f + Pulse * 0.09f),
                HSVToColor(Fraction(ThemeHue + 0.018f), 0.64f, 1.00f, 0.22f + Pulse * 0.13f),
                HSVToColor(Fraction(ThemeHue + 0.055f), 0.20f, 1.00f, 0.72f + Pulse * 0.24f)
            };

            const float Dimension = std::min(_Width, _Height);
            const float OuterWidth = std::clamp(Dimension * 0.0055f, 3.0f, 11.0f);

            for (size_t i = 0; i < TrailLayerCount; ++i)
            {
                if (!Trails[i])
                    continue;

                _TrailBrush->SetColor(TrailColors[i]);

                switch (i)
                {
                    case 0:
                        renderTarget->DrawGeometry(Trails[i].Get(), _TrailBrush.Get(), OuterWidth * (1.0f + Pulse * 0.75f), _RoundStrokeStyle.Get());
                        break;

                    case 1:
                        renderTarget->DrawGeometry(Trails[i].Get(), _TrailBrush.Get(), 2.25f + Pulse * 2.0f, _RoundStrokeStyle.Get());
                        break;

                    default:
                        renderTarget->DrawGeometry(Trails[i].Get(), _TrailBrush.Get(), 0.75f + Pulse * 0.60f, _RoundStrokeStyle.Get());
                        break;
                }
            }
        }

        // Star heads use native Direct2D ellipses. Their colors and sizes vary independently, so drawing them individually is appropriate.
        const ThreeD::prepared_projection_t PreparedProjection = ThreeD::Prepare();

        for (const star_t & Star : _Stars)
        {
            const auto ProjectedPosition = ThreeD::Project({ Star.X, Star.Y, Star.Z }, PreparedProjection);

            if (!ProjectedPosition.IsProjectable)
                continue;

            const D2D1_POINT_2F Position = ProjectedPosition.Position;

            if (Position.x < -16.f || Position.y < -16.f || Position.x > (_Width + 16.f) || Position.y > (_Height + 16.f))
                continue;

            const float Depth       = (float) Clamp(1. - (Star.Z - Projection.NearPlane) / (Projection.FarPlane - Projection.NearPlane));
            const float Brightness  = SmoothStep(0.f, 1.f, Depth);
            const float Radius      = (float) Star.Size * Lerp(0.35f, 2.45f, Brightness) * (1.f + (Pulse * Brightness * 1.20f) + ((float) _SmoothedBass * 0.22f));

            const D2D1_COLOR_F Color = GetStarColor(Star, Brightness);

            _StarBrush->SetColor(Color);

            if (Brightness > 0.62f)
            {
                D2D1_COLOR_F GlowColor = Color;

                GlowColor.a *= 0.12f;

                _StarBrush->SetColor(GlowColor);

                renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(Position.x, Position.y), Radius * 3.2f, Radius * 3.2f), _StarBrush.Get());

                _StarBrush->SetColor(Color);
            }

            renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(Position.x, Position.y), Radius, Radius), _StarBrush.Get());
        }
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::GetStarColor(const star_t & star, double depthBrightness) const noexcept
    {
        const float ColorShift = (float) _BeatPulse * 0.035f;

        const float H = Fraction(CurrentThemeHue() + (float) (star.HueOffset * 0.035f) + ColorShift + (float) (_AnimationTime * 0.0025));
        const float S = (float) Clamp(0.12 + std::abs(star.HueOffset) * 0.42 + _SmoothedBass * 0.12);
        const float V = (float) Clamp(star.Brightness * Lerp(0.30, 1.0, depthBrightness) * (1.0 + _BeatPulse * depthBrightness * 0.65));
        const float A = (float) Clamp(Lerp(0.22, 1., depthBrightness));

        return HSVToColor(H, S, V, A);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderFlash(ID2D1RenderTarget * renderTarget) noexcept
    {
        const float Radius = std::min(_Width, _Height) * (0.045f + (float) (_BeatPulse * 0.28));

        const float Opacity = (float) Clamp(0.08 + (_SmoothedBass * 0.18) + (_BeatPulse * 1.05));

        ComPtr<ID2D1RadialGradientBrush> FlashBrush;

        {
            ComPtr<ID2D1GradientStopCollection> Collection;

            const auto Color = HSVToColor(CurrentThemeHue(), 0.58f, 0.72f);

            const D2D1_GRADIENT_STOP FlashStops[] =
            {
                { 0.00f, D2D1::ColorF(  0.90f,   1.00f,   1.00f, 0.95f) },
                { 0.15f, D2D1::ColorF(Color.r, Color.g, Color.b, 0.70f) },
                { 0.48f, D2D1::ColorF(Color.r, Color.g, Color.b, 0.20f) },
                { 1.00f, D2D1::ColorF(Color.r, Color.g, Color.b, 0.00f) },
            };

            HRESULT hr = renderTarget->CreateGradientStopCollection(FlashStops, _countof(FlashStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, Collection.GetAddressOf());

            if (FAILED(hr))
                return;

            hr = renderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(_CenterX, _CenterY), D2D1::Point2F(0.f, 0.f), 64.f, 64.f), Collection.Get(), FlashBrush.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return;
        }

        FlashBrush->SetCenter(_Center);
        FlashBrush->SetGradientOriginOffset(D2D1::Point2F(0.0f, 0.0f));

        FlashBrush->SetRadiusX(Radius);
        FlashBrush->SetRadiusY(Radius);

        FlashBrush->SetOpacity(Opacity);

        renderTarget->FillEllipse(D2D1::Ellipse(_Center, Radius, Radius), FlashBrush.Get());

    }

    /// <summary>
    /// Generates the Direct2D geometries for the star trails.
    /// </summary>
    void effect_t::GenerateStarTrails(std::array<ComPtr<ID2D1PathGeometry>, TrailLayerCount> & geometries)
    {
        std::array<ComPtr<ID2D1GeometrySink>, TrailLayerCount> Sinks;

        for (size_t i = 0; i < TrailLayerCount; ++i)
        {
            if (FAILED(_D2DFactory->CreatePathGeometry(&geometries[i])))
                continue;

            if (FAILED(geometries[i]->Open(&Sinks[i])))
            {
                geometries[i].Reset();
                continue;
            }

            Sinks[i]->SetFillMode(D2D1_FILL_MODE_WINDING);
        }

        const double Pulse = Clamp(_BeatPulse);

        const ThreeD::prepared_projection_t PreparedProjection = ThreeD::Prepare();

        for (const star_t & Star : _Stars)
        {
            // Extend the tail backward along the Z axis. Beat pulses enlarge this distance, producing the forward-jump sensation.
            const double trailDepth = Lerp(0.035, 0.24, Pulse) * Star.Speed * (1.0 + (_SmoothedRMS * 2.4) + (_SmoothedBass * 0.65));
            const double tailZ      = std::min(Projection.FarPlane, std::max(Star.PreviousZ, Star.Z + trailDepth));

            point3d_t HeadPoint { Star.X, Star.Y, Star.Z };
            point3d_t TailPoint { Star.X, Star.Y, tailZ };

            if (!ClipLineToNearPlane(HeadPoint, TailPoint, PreparedProjection))
                continue;

            const auto ProjectedHead = ThreeD::Project(HeadPoint, PreparedProjection);
            const auto ProjectedTail = ThreeD::Project(TailPoint, PreparedProjection);

            if (!ProjectedHead.IsProjectable || !ProjectedTail.IsProjectable)
                continue;

            const D2D1_POINT_2F Head = ProjectedHead.Position;
            const D2D1_POINT_2F Tail = ProjectedTail.Position;

            const bool IsVisible = !(Head.x < -64.f && Tail.x < -64.f) && !(Head.y < -64.f && Tail.y < -64.f) && !(Head.x > (_Width + 64.f) && Tail.x > (_Width + 64.f)) && !(Head.y > (_Height + 64.f) && Tail.y > (_Height + 64.f));

            if (!IsVisible)
                continue;

            const double Depth = Clamp(1. - (Star.Z - Projection.NearPlane) / (Projection.FarPlane - Projection.NearPlane));

            const size_t Layer = (size_t) (Depth > 0.68f ? 2 : Depth > 0.30f ? 1 : 0);

            if (!Sinks[Layer])
                continue;

            Sinks[Layer]->BeginFigure(D2D1::Point2F(Tail.x, Tail.y), D2D1_FIGURE_BEGIN_HOLLOW);

            Sinks[Layer]->AddLine(D2D1::Point2F(Head.x, Head.y));

            Sinks[Layer]->EndFigure(D2D1_FIGURE_END_OPEN);
        }

        for (auto & Sink : Sinks)
        {
            if (Sink)
                Sink->Close();
        }
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr =__super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if ((_RenderTarget == renderTarget) && _StarBrush && _TrailBrush && _BackgroundBrush)
            return S_OK;

        renderTarget->GetFactory(_D2DFactory.ReleaseAndGetAddressOf());

        if (!_D2DFactory)
            return E_FAIL;

        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), _StarBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.4f, 0.8f, 1.0f, 1.0f), _TrailBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.05f), _ScanlineBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.35f, 0.5f, 0.25f), _FrameBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        {
            const D2D1_GRADIENT_STOP BackgroundStops[] =
            {
                { 0.00f, D2D1::ColorF(0.018f, 0.060f, 0.125f, 1.00f) },
                { 0.30f, D2D1::ColorF(0.008f, 0.022f, 0.060f, 0.95f) },
                { 0.72f, D2D1::ColorF(0.002f, 0.006f, 0.018f, 0.75f) },
                { 1.00f, D2D1::ColorF(0.000f, 0.000f, 0.003f, 0.00f) }
            };

            hr = renderTarget->CreateGradientStopCollection(BackgroundStops, _countof(BackgroundStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, _BackgroundStops.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;

            hr = renderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(_CenterX, _CenterY), D2D1::Point2F(0.0f, 0.0f), _CenterX, _CenterY), _BackgroundStops.Get(), _BackgroundBrush.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f);

            hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _RoundStrokeStyle.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        _RenderTarget = renderTarget;

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _RenderTarget = nullptr;

        _StarBrush.Reset();
        _TrailBrush.Reset();
        _ScanlineBrush.Reset();
        _FrameBrush.Reset();

        _BackgroundBrush.Reset();
        _BackgroundStops.Reset();

        _RoundStrokeStyle.Reset();

        _D2DFactory.Reset();

        __super::DeleteDeviceDependentResources();
    }
}

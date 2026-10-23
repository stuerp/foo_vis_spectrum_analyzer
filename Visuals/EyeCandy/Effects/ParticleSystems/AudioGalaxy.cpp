
/** $VER: AudioGalaxy.cpp (2026.10.23) **/

#include <pch.h>

#include "AudioGalaxy.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>

namespace AudioGalaxy
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
        return std::format(L"Audio Galaxy, {} quality, {} speed", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept
    {
        const D2D1_SIZE_F Viewport = renderTarget->GetSize();

        const float ViewportWidth  = std::max(1.f, Viewport.width);
        const float ViewportHeight = std::max(1.f, Viewport.height);

        // Recreate size-dependent brushes immediately after a viewport resize. Projection and galaxy coverage are updated in the same frame.
        if ((std::abs(ViewportWidth - _ViewportWidth) > .5f) || (std::abs(ViewportHeight - _ViewportHeight) > .5f))
        {
            _ViewportWidth  = ViewportWidth;
            _ViewportHeight = ViewportHeight;
            _Width  = ViewportWidth;
            _Height = ViewportHeight;

            DeleteDeviceDependentResources();
        }

        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = std::clamp(deltaTime, 0.0, 0.1);

        Projection.Width       = (double) ViewportWidth;
        Projection.Height      = (double) ViewportHeight;
        Projection.AspectRatio = (double) ViewportWidth / (double) ViewportHeight;

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

                Camera.Target = { 0., 0., .72 };

                // Roll the camera basis rather than the completed 2D image. Projection and clipping now use the
                // same banked coordinate frame, avoiding the loss of fly-past motion at large roll angles.
                const double Roll = _CameraRoll * Pi / 180.;

                Camera.Up = { std::sin(Roll), std::cos(Roll), 0. };

                const ThreeD::prepared_projection_t PreparedProjection = ThreeD::Prepare();

                const projected_point_t ProjectedCenter = ThreeD::Project({ 0., 0., .72 }, PreparedProjection);

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
    void effect_t::AnalyzeAudio(const audio_chunk_impl & chunk)
    {
        _Peak         = 0.;
        _RMS          = 0;

        _BassEnergy   = 0;
        _MidEnergy    = 0;
        _TrebleEnergy = 0.;

        _HasAudio = false;

        if (chunk.is_empty())
            return;

        const size_t FrameCount = chunk.get_sample_count(), ChannelCount = chunk.get_channel_count(), SampleRate = chunk.get_sample_rate();
        const size_t MaxFrames = std::min<size_t>(FrameCount, 4096), FrameIndex = FrameCount - MaxFrames;

        if ((MaxFrames < 8) || !ChannelCount || !SampleRate)
            return;

        const audio_sample * Frame = chunk.get_data() + FrameIndex * ChannelCount;
        const audio_sample * const End = Frame + MaxFrames * ChannelCount;

        double Sum = 0., SumBass = 0., SumMid = 0., SumTreble = 0.;

        const double DC = std::exp(-TwoPi * 35. / (double) SampleRate), Low = std::exp(-TwoPi * 220. / (double) SampleRate), High = std::exp(-TwoPi * 2400. / (double) SampleRate);

        while (Frame < End)
        {
            double Mono = 0.;

            for (size_t Channel = 0; Channel < ChannelCount; ++Channel)
                Mono += Frame[Channel];

            Frame += ChannelCount;

            const double Sample = ClampAudio(Mono / (double) ChannelCount);

            _Peak = std::max(_Peak, std::abs(Sample)); Sum += Sample * Sample;

            _BassState = (1. - Low) * Sample + Low * _BassState;
            _DCState   = (1. - DC) * _BassState + DC * _DCState;
            _MidState  = (1. - High) * Sample + High * _MidState;

            const double Bass   = _BassState - _DCState;
            const double Mid    = _MidState - _BassState;
            const double Treble = Sample - _MidState;

            SumBass   += Bass   * Bass;
            SumMid    += Mid    * Mid;
            SumTreble += Treble * Treble;
        }

        const double Scale = 1. / (double) MaxFrames;

        _RMS = std::sqrt(Sum * Scale);

        _BassEnergy   = std::clamp(std::log1p(std::sqrt(SumBass   * Scale) * 36.) / std::log1p(36.), 0., 1.);
        _MidEnergy    = std::clamp(std::log1p(std::sqrt(SumMid    * Scale) * 22.) / std::log1p(22.), 0., 1.);
        _TrebleEnergy = std::clamp(std::log1p(std::sqrt(SumTreble * Scale) * 16.) / std::log1p(16.), 0., 1.);

        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);
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
                    return 0.75;

                default:
                case EyeCandy::Speed::Normal:
                    return 1.00;

                case EyeCandy::Speed::Fast:
                    return 1.75;
            }
        }();

        _AnimationTime += SpeedFactor * deltaTime;
        _TransitionTime += deltaTime;

        while (_TransitionTime >= TransitionInterval)
        {
            _TransitionTime -= TransitionInterval;

            _ThemeIndex = (_ThemeIndex + 1) % ThemeCount;
        }

        _SmoothedPeak   = ExponentialSmoothing(_SmoothedPeak,   _Peak,         deltaTime, (_Peak         > _SmoothedPeak)   ? 34. : 6.5);
        _SmoothedRMS    = ExponentialSmoothing(_SmoothedRMS,    _RMS,          deltaTime, (_RMS          > _SmoothedRMS)    ? 18. : 4.);

        _SmoothedBass   = ExponentialSmoothing(_SmoothedBass,   _BassEnergy,   deltaTime, (_BassEnergy   > _SmoothedBass)   ? 24. : 5.);
        _SmoothedMid    = ExponentialSmoothing(_SmoothedMid,    _MidEnergy,    deltaTime, (_MidEnergy    > _SmoothedMid)    ? 20. : 4.5);
        _SmoothedTreble = ExponentialSmoothing(_SmoothedTreble, _TrebleEnergy, deltaTime, (_TrebleEnergy > _SmoothedTreble) ? 30. : 7.);

        const double Strength = std::clamp(std::max(0., _Peak - _PreviousPeak) * 5.5 + std::max(0., _TrebleEnergy - _PreviousTreble) * 4., 0., 1.);

        _PreviousPeak = _Peak; _PreviousTreble = _TrebleEnergy;
        _TransientCooldown = std::max(0., _TransientCooldown - deltaTime); _TransientPulse *= std::exp(-deltaTime * 9.);

        // Transients create a compact group of genuinely new stars at the galactic core.
        if (_HasAudio && (_TransientCooldown <= 0.) && (Strength > .11))
        {
            _TransientPulse = std::max(_TransientPulse, Strength); _TransientCooldown = .045;

            const size_t Count = std::min<size_t>(_Stars.size(), 4 + (size_t)std::lround(Strength * 22.));

            for (size_t i = 0; i < Count; ++i)
            {
                RespawnStar(_Stars[_TransientStarIndex], _TransientStarIndex, false);

                _TransientStarIndex = (_TransientStarIndex + 1) % _Stars.size();
            }
        }

        _BeatPulse = ExponentialSmoothing(_BeatPulse, _TransientPulse, deltaTime, 18.);

        _CameraRollVelocity = ExponentialSmoothing(_CameraRollVelocity, 1.8 + _SmoothedMid * 5.2, deltaTime, 2.8);
        _CameraRoll += _CameraRollVelocity * deltaTime * SpeedFactor; if (_CameraRoll > 180.) _CameraRoll -= 360.;

        const double RotationSpeed = (.08 + _SmoothedMid * .22 + _SmoothedTreble * .12) * SpeedFactor;

        _TravelDistance += RotationSpeed * deltaTime; GenerateWork(deltaTime, RotationSpeed);
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
        _DCState = 0.;
        _MidState = 0.;
        _TrebleEnergy = _SmoothedTreble = 0.;
        _PreviousPeak = _PreviousTreble = 0.;
        _TransientPulse = _TransientCooldown = 0.;
        _TransientStarIndex = 0;

        _MidEnergy = 0.;

        _SmoothedMid = 0.;
        _PreviousMid = 0.;

        _MidDirectionCooldown = 0.;

        _CameraRollVelocity = 0.;
        _CameraRollDirection = 1.;

        _FlightYaw = 0.;
        _FlightPitch = 0.;
        _FlightYawVelocity = 0.;
        _FlightPitchVelocity = 0.;

        _PreviousBass = 0.;
        _BeatAverage = 0.;
        _BeatPulse = 0.;
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
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, &Generation]() noexcept
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

        Star.PreviousX = Star.X;
        Star.PreviousY = Star.Y;
        Star.PreviousZ = Star.Z;

        Star.Age    += deltaTime;
        Star.Angle  += forwardSpeed * Star.Speed * deltaTime * (.65 + (1. - Star.Radius) * 1.8);
        Star.Radius += deltaTime * (.004 + _SmoothedBass * .010) * Star.Speed;

        const double AspectRatio = (_ViewportHeight > 0.f) ? (double) _ViewportWidth / (double) _ViewportHeight : 1.;
        const double ScaleX      = std::max(1., AspectRatio) * 1.08;
        const double ScaleY      = std::max(1., 1. / AspectRatio) * 1.08;
        const double ArmAngle    = Star.Angle + Star.Radius * 8.5 + (double) Star.Band * (TwoPi / 3.);
        const double Energy      = (Star.Band == 0) ? _SmoothedBass : (Star.Band == 1) ? _SmoothedMid : _SmoothedTreble;
        const double Ripple      = std::sin(Star.Angle * 3. + Star.Seed * .0001) * (.035 + (1. - Energy) * .075);

        // Each star follows its own inclined orbital plane. A central bulge and a radius-dependent halo create real volume instead of a flat disk.
        const double OrbitalX  = std::cos(ArmAngle + Ripple) * Star.Radius;
        const double OrbitalY  = std::sin(ArmAngle + Ripple) * Star.Radius;

        const double InclinedY = OrbitalY * std::cos(Star.Inclination) - Star.Height * std::sin(Star.Inclination);
        const double InclinedZ = OrbitalY * std::sin(Star.Inclination) + Star.Height * std::cos(Star.Inclination);

        const double LocalX    = OrbitalX * std::cos(Star.NodeAngle) - InclinedY * std::sin(Star.NodeAngle);
        const double LocalY    = OrbitalX * std::sin(Star.NodeAngle) + InclinedY * std::cos(Star.NodeAngle);
        const double LocalZ    = InclinedZ;

        // Slowly rotate the complete three-dimensional galaxy around all three axes.
        const double Pitch = .38 + std::sin(_AnimationTime * .13) * .16;
        const double Yaw   = _AnimationTime * .10;
        const double Roll  = _AnimationTime * .06;

        const double PitchY = LocalY * std::cos(Pitch) - LocalZ * std::sin(Pitch);
        const double PitchZ = LocalY * std::sin(Pitch) + LocalZ * std::cos(Pitch);

        const double YawX = LocalX * std::cos(Yaw) + PitchZ * std::sin(Yaw);
        const double YawZ = -LocalX * std::sin(Yaw) + PitchZ * std::cos(Yaw);

        const double RollX = YawX * std::cos(Roll) - PitchY * std::sin(Roll);
        const double RollY = YawX * std::sin(Roll) + PitchY * std::cos(Roll);

        Star.X = RollX * ScaleX;
        Star.Y = RollY * ScaleY;
        Star.Z = .82 + YawZ * .30;

        if ((Star.Age >= Star.Lifetime) || (Star.Radius > 1.30))
            RespawnStar(Star, index, false);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RespawnStar(star_t & star, size_t index, bool randomDepth) noexcept
    {
        uint32_t State = Hash((uint32_t)index * 0x9E3779B9u ^ star.Seed ^ (uint32_t)(_TravelDistance * 100000.f)); star.Seed = State;

        const double BW = .2 + _SmoothedBass;
        const double MW = .2 + _SmoothedMid;
        const double TW = .2 + _SmoothedTreble;

        const double Pick = Random(State) * (BW + MW + TW);

        star.Band       = (Pick < BW) ? 0u : (Pick < BW + MW) ? 1u : 2u;
//      star.Radius     = randomDepth ? Lerp(.055, 2.22, std::pow((double) Random(State), .72)) : Lerp(.035, 1.18, (double) Random(State));
        star.Radius     = randomDepth ? Lerp(.055, 4.22, std::pow((double) Random(State), .72)) : Lerp(.035, 3.18, (double) Random(State));
        star.Angle      = Random(State) * TwoPi;
        star.Speed      = Lerp(.72, 1.35, (double) Random(State));
//      star.Size       = Lerp(.55, 1.55, (double) Random(State));
        star.Size       = Lerp(.55, 5.55, (double) Random(State));
        star.Brightness = randomDepth ? Lerp(.25, .90, (double)Random(State)) : Lerp(1.35, 1.85, (double)Random(State)); star.HueOffset = RandomSigned(State);
        star.Age        = randomDepth ? Random(State) * 10. : 0.; star.Lifetime = Lerp(7., 18., (double)Random(State));

        const double Thickness = .035 + (1. - std::min(1., star.Radius)) * .18;

        star.Height      = RandomSigned(State) * Thickness * (0.35 + std::pow(std::abs(RandomSigned(State)), .55));
        star.Inclination = RandomSigned(State) * (.12 + (1. - std::min(1., star.Radius)) * .48);
        star.NodeAngle   = Random(State) * TwoPi;

        const double A = star.Angle + star.Radius * 8.5 + (double) star.Band * (TwoPi / 3.); star.X = std::cos(A) * star.Radius; star.Y = std::sin(A) * star.Radius; star.Z = .82 + star.Height * .30;

        star.PreviousX = star.X; star.PreviousY = star.Y; star.PreviousZ = star.Z;
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

        _BackgroundBrush->SetOpacity((float) std::clamp(0.72 + (_SmoothedRMS * 0.4) + (_BeatPulse * 0.18), 0., 1.));

        renderTarget->FillRectangle(_Rectangle, _BackgroundBrush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderStars(ID2D1RenderTarget * renderTarget)
    {
        std::array<ComPtr<ID2D1PathGeometry>, TrailLayerCount> Trails;

        const float Pulse = (float) std::clamp(_BeatPulse, 0., 1.);

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
            const projected_point_t ProjectedPosition = ThreeD::Project({ Star.X, Star.Y, Star.Z }, PreparedProjection);

            if (!ProjectedPosition.IsProjectable)
                continue;

            const D2D1_POINT_2F Position = ProjectedPosition.Position;

            if (Position.x < -16.0f || Position.y < -16.0f || Position.x > _Width + 16.0f || Position.y > _Height + 16.0f)
                continue;

            const float Depth       = (float) std::clamp(1. - Star.Radius / 1.35, 0., 1.);
            const float BirthGlow   = (float) std::exp(-Star.Age * 2.15);
            const float Brightness  = std::clamp(.30f + SmoothStep(0.f, 1.f, Depth) * .45f + BirthGlow * .42f, 0.f, 1.0f);
            const float Radius      = (float) Star.Size * Lerp(0.35f, 2.45f, Brightness) * (1.f + (Pulse * Brightness * 1.20f) + ((float) _SmoothedBass * 0.22f));

            const D2D1_COLOR_F Color = GetStarColor(Star, Brightness);
            const float Feather = (Brightness > 0.62f) ? 2.8f + (BirthGlow * 1.4f) : 1.65f;

            DrawStar(renderTarget, Position, Radius, Feather, Color);
        }
    }

    /// <summary>
    /// Draws a star with a solid core and an alpha-feathered outer glow.
    /// </summary>
    void effect_t::DrawStar(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F position, float radius, float feather, D2D1_COLOR_F color) noexcept
    {
        const float OuterRadius = radius * feather;
        const float CoreEdge = std::clamp(1.f / feather, 0.08f, 0.92f);

        ComPtr<ID2D1GradientStopCollection> Collection;
        ComPtr<ID2D1RadialGradientBrush> Brush;

        // Keep the star solid through its visible body, then quickly fade its color and alpha near the outside of the circle.
        const D2D1_GRADIENT_STOP Stops[] =
        {
            { 0.00f,                      D2D1::ColorF(color.r,         color.g,         color.b,         color.a) },
            { CoreEdge * 0.80f,           D2D1::ColorF(color.r,         color.g,         color.b,         color.a) },
            { CoreEdge,                   D2D1::ColorF(color.r * 0.96f, color.g * 0.96f, color.b * 0.96f, color.a * 0.55f) },
            { Lerp(CoreEdge, 1.f, 0.18f), D2D1::ColorF(color.r * 0.82f, color.g * 0.82f, color.b * 0.82f, color.a * 0.07f) },
            { Lerp(CoreEdge, 1.f, 0.38f), D2D1::ColorF(color.r * 0.68f, color.g * 0.68f, color.b * 0.68f, 0.00f) },
            { 1.00f,                      D2D1::ColorF(color.r * 0.68f, color.g * 0.68f, color.b * 0.68f, 0.00f) },
        };

        HRESULT hr = renderTarget->CreateGradientStopCollection(Stops, _countof(Stops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, Collection.GetAddressOf());

        if (FAILED(hr))
            return;

        hr = renderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(position, D2D1::Point2F(0.f, 0.f), OuterRadius, OuterRadius), Collection.Get(), Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return;

        renderTarget->FillEllipse(D2D1::Ellipse(position, OuterRadius, OuterRadius), Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::GetStarColor(const star_t & star, double depthBrightness) const noexcept
    {
        const float ColorShift = (float) _BeatPulse * 0.035f;

        const float H = Fraction(CurrentThemeHue() + (float) (star.HueOffset * 0.035f) + ColorShift + (float) (_AnimationTime * 0.0025));
        const float S = (float) std::clamp(0.12 + std::abs(star.HueOffset) * 0.42 + _SmoothedBass * 0.12, 0., 1.);
        const float V = (float) std::clamp(star.Brightness * Lerp(0.30, 1.15, depthBrightness) * (1.0 + _BeatPulse * depthBrightness * 0.85), 0., 1.);
        const float A = (float) std::clamp(Lerp(0.22, 1., depthBrightness), 0., 1.);

        return HSVToColor(H, S, V, A);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderFlash(ID2D1RenderTarget * renderTarget) noexcept
    {
        const float Radius = std::min(_Width, _Height) * (0.025f + (float) (_BeatPulse * 0.12));

        const float Opacity = (float) std::clamp(0.025 + (_SmoothedBass * 0.06) + (_BeatPulse * 0.28), 0., 1.);

        ComPtr<ID2D1RadialGradientBrush> FlashBrush;

        {
            ComPtr<ID2D1GradientStopCollection> Collection;

            const auto Color = HSVToColor(CurrentThemeHue(), 0.58f, 0.72f);

            const D2D1_GRADIENT_STOP FlashStops[] =
            {
                { 0.00f, D2D1::ColorF(  0.80f,   0.92f,   1.00f, 0.38f) },
                { 0.15f, D2D1::ColorF(Color.r, Color.g, Color.b, 0.28f) },
                { 0.48f, D2D1::ColorF(Color.r, Color.g, Color.b, 0.07f) },
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
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr =__super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if ((_RenderTarget == renderTarget) && _TrailBrush && _BackgroundBrush)
            return S_OK;

        renderTarget->GetFactory(_D2DFactory.ReleaseAndGetAddressOf());

        if (!_D2DFactory)
            return E_FAIL;

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

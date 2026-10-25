
/** $VER: NeonTunnel.cpp (2026.10.23) **/

#include <pch.h>

#include "NeonTunnel.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

namespace NeonTunnel
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
        return std::format(L"Neon Tunnel, {}, {} quality, {} speed", GetThemeName(_ThemeFrom).c_str(), Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
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
        GenerateWork();

        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        RenderBackground(renderTarget);

        {
            const float cx = _CenterX;
            const float cy = _CenterY;

            const float ShakeX = (float) (std::sin(_AnimationTime * 31.) * (_CameraShake * 2.2));
            const float ShakeY = (float) (std::cos(_AnimationTime * 27.) * (_CameraShake * 1.8));

            {
                renderTarget->SetTransform(D2D1::Matrix3x2F::Rotation((float) _CameraRoll, D2D1::Point2F(cx, cy)) * D2D1::Matrix3x2F::Translation(ShakeX, ShakeY));

                RenderTunnel(renderTarget);
                RenderCoreGlow(renderTarget);
                RenderBeatFlash(renderTarget);

                renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            }
        }

        RenderScanlines(renderTarget);
    
        // Draw a border around the full window.
        _FrameBrush->SetColor(GetThemeColor(ThemeRole::Ring, 0.12f, 0.82f, 0.80f, (float) Clamp(0.16 + (_BeatPulse * 0.20) + (_BassImpulse * 0.18))));

        renderTarget->DrawRectangle(D2D1::RectF(0.5f, 0.5f, _Width - 0.5f, _Height - 0.5f), _FrameBrush.Get(), 1.f);
    }

    /// <summary>
    /// Aanalyzes an audio chunk.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _Peak         = 0.;
        _RMS          = 0.;

        _BassEnergy   = 0.;
        _MidEnergy    = 0.;
        _TrebleEnergy = 0.;

        _Bands.fill(0.);

        _HasAudio = false;

        if (chunk.is_empty())
            return;

        constexpr size_t MaximumFrameCount = 2048;

        const size_t SampleCount    = chunk.get_sample_count();
        const size_t ChannelCount   = chunk.get_channel_count();
        const double SampleRate     = (double) chunk.get_sample_rate();
        const audio_sample * Data   = chunk.get_data();


        // Analyze only the most recent 2048 sample frames.
        const size_t FrameCount = std::min(SampleCount, MaximumFrameCount);

        if (FrameCount < 32)
            return;

        // Determine the index
        const size_t FrameIndex = SampleCount - FrameCount;

        std::array<double, MaximumFrameCount> Samples;

        const audio_sample * Frame = Data + (FrameIndex * ChannelCount);

        const double WindowScale = TwoPi / (double) (FrameCount - 1);

        double Sum  = 0.;
        double Peak = 0.;

        for (size_t i = 0; i < FrameCount; ++i, ++Frame)
        {
            const double Sample = ClampAudio((double) Frame[0]);

            // Apply a Hann window.
            Samples[i] = Sample * (0.5 - (std::cos((double) i * WindowScale) * 0.5));

            Sum += Sample * Sample;

            Peak = std::max(Peak, std::abs(Sample));
        }

        _Peak = Peak;
        _RMS  = std::sqrt(Sum / (double) FrameCount);

        _HasAudio = (_Peak > 1e-5) || (_RMS > 1e-6);

        if (!_HasAudio)
            return;

        // Perform the frequency analysis using a Goertzel-style frequency detector at selected frequencies.
        const double NyquistFrequency   = SampleRate * 0.5;
        const double MaximumCenter      = NyquistFrequency * 0.92;
        const double MaximumProbe       = NyquistFrequency * 0.95;
        const double InverseFrameCount  = 1. / (double) FrameCount;
        const double MagnitudeScale     = 1. / std::log1p(90.0);
        const double ProbeOffset        = 1.122462048;
        const double InverseProbeOffset = 1. / ProbeOffset;

        // Calculate the logarithmic step once instead of calculating pow() for every band.
        const double CenterStep = std::pow(16'500.0 / 40.0, 1. / (double) (FrequencyBandCount - 1)); // 40 Hz to 16.5 kHz

        double Center = 45.;

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const double NormalizedBandIndex = (double) i / (double) (FrequencyBandCount - 1);

            // Distribute band centers approximately between 45 Hz and 14 kHz using logarithmic spacing.
            const double ClampedCenter = std::clamp(Center, 20.0, MaximumCenter);

            // Create three probes around each center. 1.122462048 ~= 2^1/6 or 2 semitones.
            const double Probes[] =
            {
                ClampedCenter * InverseProbeOffset,
                ClampedCenter,
                ClampedCenter * ProbeOffset
            };

            double PowerSum = 0.;

            // Analyze each probe frequency.
            for (double Probe : Probes)
            {
                Probe = std::clamp(Probe, 20.0, MaximumProbe);

                // Goertzel-style detector
                {
                    // Recurrence coefficient
                    const double Coefficient = 2. * std::cos(TwoPi * Probe / SampleRate);

                    double q1 = 0.;
                    double q2 = 0.;

                    const double * Sample = Samples.data();
                    const double * SampleEnd = Sample + FrameCount;

                    // Process all windowed samples.
                    while (Sample != SampleEnd)
                    {
                        const double q0 = *Sample++ + Coefficient * q1 - q2;

                        q2 = q1;
                        q1 = q0;
                    }

                    // Convert the recurrence to power.
                    PowerSum += std::max(0., (q2 * q2) + (q1 * q1) - Coefficient * q1 * q2);
                }
            }

            // Calculate the band magnitude.
            const double Magnitude = std::sqrt(PowerSum / (double) std::size(Probes)) * InverseFrameCount;

            // Compress and shape the band magnitude to a value in the [0, 1] range.
            _Bands[i] = Clamp(std::log1p(Magnitude * 90.) * MagnitudeScale * Lerp(0.85, 1.45, NormalizedBandIndex));

            Center *= CenterStep;
        }

        // Bass emphasizes the deepest bass band (48%). 48% + 34% + 18% = 100%
        const double Bass = (_Bands[0] * 0.48) + (_Bands[1] * 0.34) + (_Bands[2] * 0.18);

        _BassEnergy   = Clamp(1. - std::exp(-Bass * 2.75 * _BassResponse)); // Apply a nonlinear bass response.
        _MidEnergy    = Clamp((_Bands[3] + _Bands[4] + _Bands[5]) / 3.);    // Average the middle bands
        _TrebleEnergy = Clamp((_Bands[6] + _Bands[7]) * 0.5);               // Average the treble bands.
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime)
    {
        if (_Rings.empty())
            Reset();

        UpdateThemeTransition(deltaTime);

        // Speeds up the animation.
        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case EyeCandy::Speed::Slow:
                    return 0.50;

                case EyeCandy::Speed::Fast:
                    return 1.50;

                case EyeCandy::Speed::Normal:
                default:
                    return 1.00;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        // Apply temporal smoothing. Choose a different response rate based on a rising or falling signal.
        {
            _SmoothedPeak   = ExponentialSmoothing(_SmoothedPeak,   _Peak,         deltaTime, (_Peak         > _SmoothedPeak)   ? 24. : 5.); // A loud transient becomes visible quickly, but its visual influence fades more gradually.
            _SmoothedRMS    = ExponentialSmoothing(_SmoothedRMS,    _RMS,          deltaTime, (_RMS          > _SmoothedRMS)    ? 18. : 4.);

            _SmoothedBass   = ExponentialSmoothing(_SmoothedBass,   _BassEnergy,   deltaTime, (_BassEnergy   > _SmoothedBass)   ? 34. : 4.); // A bass hit causes an almost immediate visual response, followed by a slower decay.
            _SmoothedMid    = ExponentialSmoothing(_SmoothedMid,    _MidEnergy,    deltaTime, (_MidEnergy    > _SmoothedMid)    ? 20. : 5.);
            _SmoothedTreble = ExponentialSmoothing(_SmoothedTreble, _TrebleEnergy, deltaTime, (_TrebleEnergy > _SmoothedTreble) ? 18. : 6.);

            // Smooth the individual bands. Can be used for changing individual ring segments, modifying rail brightness, controlling colors, deforming geometry by frequency band.
            for (size_t i = 0; i < FrequencyBandCount; ++i)
                _SmoothedBands[i] = ExponentialSmoothing(_SmoothedBands[i], _Bands[i], deltaTime, _Bands[i] > _SmoothedBands[i] ? 20. : 5.);
        }

        // Beat followers. A possible beat occurs when the current fast envelope rises significantly above the slow average.
        _BeatAverage  = ExponentialSmoothing(_BeatAverage,  _BassEnergy, deltaTime, 1.45);                                      // Slow bass follower (moving average), What bass level is normal recently?
        _BeatEnvelope = ExponentialSmoothing(_BeatEnvelope, _BassEnergy, deltaTime, (_BassEnergy > _BeatEnvelope) ? 38. : 8.);  // Fast bass follower (captures the shape of the bass), What is the bass doing right now?

        // Bass attack detection. Continuous bass tone: Envelope stays high, but Attack approaches zero. Kick drum onset: nvelope rises abruptly, so Attack becomes large.
        const double Attack = std::max(0., _BeatEnvelope - _PreviousBassEnvelope);

        _PreviousBassEnvelope = _BeatEnvelope;

        // Beat cooldown (in seconds). When set prevents another beat detection for 115 ms. Max. trigger rate is 1 / 0.115 ~ 8.7 detected beats per second.
        {
            _BeatCooldown = std::max(0., _BeatCooldown - deltaTime);

            const double Threshold = std::max(0.11, _BeatAverage * 1.38 + 0.03); // The current bass envelope must be approximately 38% above the moving average. 0.11 is the noise floor.

            if (_HasAudio && (_BeatCooldown <= 0.) && (_BeatEnvelope > Threshold) && (_SmoothedPeak > 0.055))
            {
                // Map the amount above the threshold into a pulse strength.
                const auto NormalizedExcess = (_BeatEnvelope - Threshold) / std::max(0.001, 1. - Threshold);

                _BeatPulse    = Clamp((0.55 + (NormalizedExcess * 1.8)) * _BeatResponse);
                _BeatCooldown = 0.115;
            }
            else
                _BeatPulse *= std::exp(-deltaTime * 7.3); // Decay the pulse strength.
        }

        // Bass impulse. It combines two different representations of a beat: Attack, immediate frame-to-frame bass acceleration. _BeatPulse, more stable contribution (validated beat-event strength).
        const double ImpulseTarget = Clamp((Attack * 8. * _BassResponse) + (_BeatPulse * 0.82 * _BeatResponse));

        {
            constexpr double AttackResponse  = 48.; // A strong kick should produce a nearly immediate visual impact.
            constexpr double ReleaseResponse = 10.; // Slower, prevents the effect from vanishing in one frame.

            _BassImpulse = ExponentialSmoothing(_BassImpulse, ImpulseTarget, deltaTime, (ImpulseTarget > _BassImpulse) ? AttackResponse : ReleaseResponse);
        }

        // Tunnel expansion.
        {
            const auto SustainedComponent = _SmoothedBass * 1.30; // Continuous bass causes the tunnel to remain expanded.
            const auto TransientComponent = _BassImpulse  * 0.82; // A sudden kick adds an extra expansion burst.

            const double Expansion = std::clamp(SustainedComponent + TransientComponent, 0., 1.65);

            constexpr double AttackResponse  = 30.; // Kick arrives: tunnel rapidly opens.
            constexpr double ReleaseResponse =  7.; // Kick ends: tunnel gradually contracts

            _BassExpansion = ExponentialSmoothing(_BassExpansion, Expansion, deltaTime, (Expansion > _BassExpansion) ? AttackResponse : ReleaseResponse);
        }

        // Tunnel twist. Bass makes the tunnel spin faster rather than rotating it to a fixed bass-dependent angle.
        {
            const double Twist = std::clamp(_SmoothedBass * 0.90 + _BassImpulse * 0.65, 0., 1.35);

            _BassTwist = ExponentialSmoothing(_BassTwist, Twist, deltaTime, Twist > _BassTwist ? 24. : 5.5);
        }

        // Center-kick trigger. A strong rising bass event pushes the complete tunnel center in a new pseudo-random direction. Edge detection prevents sustained bass from applying a new push every frame.
        {
            const auto TransientComponent        = _BassImpulse * 0.90;
            const auto validatedBeatContribution = _BeatPulse   * 0.30f;
            const auto ImmediateOnsetSensitivity = Attack * 5.5;            // Heavily amplified because the center kick should be mainly onset-driven. Sustained bass should not keep pushing the tunnel continuously.

            const double CenterKickTrigger = Clamp(TransientComponent + validatedBeatContribution + ImmediateOnsetSensitivity);

            // Rising-edge detection
            {
                constexpr auto TriggerTreshold = 0.46;
                constexpr auto ReleaseTreshold = 0.18;

                const bool IsCenterKicked = (CenterKickTrigger >= TriggerTreshold) && (_PreviousCenterKickTrigger < TriggerTreshold);

                if (IsCenterKicked)
                {
                    // Determine a pseudo-random kick direction.
                    _CenterKickSeed = Hash(_CenterKickSeed ^ (uint32_t)(_TravelDistance * 131071.0f) ^ (uint32_t)(_AnimationTime * 1'000.0));

                    const double Direction = Random(_CenterKickSeed) * TwoPi;

                    // Determine center-kick strength and velocity.
                    const double Strength  = std::clamp(0.58 + (CenterKickTrigger * 1.05) + (_BassImpulse * 0.55), 0., 1.85);

                    _CenterKickVelocityX += std::cos(Direction) * Strength;
                    _CenterKickVelocityY += std::sin(Direction) * Strength;
                }

                // Trigger release. After triggering, the signal needs to fall substantially before another kick can be detected.
                _PreviousCenterKickTrigger = (CenterKickTrigger <= ReleaseTreshold) ? 0. : CenterKickTrigger;
            }
        }

        // Critically damped second-order center recovery. This formulation avoids a visible snap when the kick ends and remains well behaved when a render frame is unusually long.
        // A spring system may be:
        // - underdamped: oscillates around the resting position
        // - overdamped: returns slowly without oscillation
        // - critically damped: returns as fast as possible without oscillating
        {
            constexpr double RecoveryFrequency = 2.20; // Hz

            constexpr double Omega = TwoPi * RecoveryFrequency;
            constexpr double OmegaSquared = Omega * Omega;

            const double Denominator = 1.0 + (2.0 * Omega * deltaTime) + (OmegaSquared * deltaTime * deltaTime); // x′′ + 2ωx′ + ω2x = 0

            const double OldKickX = _CenterKickX;
            const double OldKickY = _CenterKickY;

            // Update the center position.
            _CenterKickX = (_CenterKickX * (1.0 + (2.0 * Omega * deltaTime)) + (_CenterKickVelocityX * deltaTime)) / Denominator;
            _CenterKickY = (_CenterKickY * (1.0 + (2.0 * Omega * deltaTime)) + (_CenterKickVelocityY * deltaTime)) / Denominator;

            // Update the center velocity.
            _CenterKickVelocityX = (_CenterKickVelocityX - (OmegaSquared * deltaTime * OldKickX)) / Denominator;
            _CenterKickVelocityY = (_CenterKickVelocityY - (OmegaSquared * deltaTime * OldKickY)) / Denominator;

            /* Prevent several closely spaced kicks from accumulating forever. Artistic and numerical safe guards */
            _CenterKickX = std::clamp(_CenterKickX, -1.25, 1.25);
            _CenterKickY = std::clamp(_CenterKickY, -1.25, 1.25);

            _CenterKickVelocityX = std::clamp(_CenterKickVelocityX, -3.5, 3.5);
            _CenterKickVelocityY = std::clamp(_CenterKickVelocityY, -3.5, 3.5);
        }

        // A subtle angular sway follows a separate set of slow sinusoidal phases. Bass adds a small temporary rotational accent, but the shared angle keeps all rings and rails coherent.
        {
            // A sum of three slow sine waves plus a center-kick contribution.
            const double CenterRotationTarget = (std::sin(_AnimationTime * 0.370f + 0.45f) * 0.055f) +       // Primary sway
                                                (std::sin(_AnimationTime * 0.190f + 2.10f) * 0.032f) +       // Subtle irregularity
                                                (std::sin(_AnimationTime * 0.071f + 4.30f) * 0.018f) +       // Subtle irregularity
                                                (_CenterKickX * 0.035f) - (_CenterKickY * 0.025f);           // Small rotational coupling

            _CenterRotationSway = ExponentialSmoothing(_CenterRotationSway, CenterRotationTarget, deltaTime, 5.);
        }

        // Rotation-direction switching
        {
            _RotationSwitchCooldown = std::max(0., _RotationSwitchCooldown - deltaTime);

            {
                // Rotation reversal is mainly caused by a strong transient, not ordinary continuous bass.
                const double Trigger = Clamp((_BassImpulse * 0.72) + (_BeatPulse * 0.20) + (_SmoothedBass * 0.18));

                if (Trigger >= RotationSwitchThreshold && _PreviousRotationTrigger < RotationSwitchThreshold && _RotationSwitchCooldown <= 0.)
                {
                    _RotationDirectionTarget = -_RotationDirectionTarget;
                    _RotationSwitchCooldown = RotationSwitchCooldownSeconds;
                }

                _PreviousRotationTrigger = (Trigger <= RotationSwitchRelease) ? 0. : Trigger;
            }

            // Smoothing causes the tunnel rotation to slow down, stops briefly, and accelerates in the opposite direction.
            _RotationDirection = ExponentialSmoothing(_RotationDirection, _RotationDirectionTarget, deltaTime, 5.);
        }

        // Camera shake.
        {
            // Main component is _BassImpulse, so shake is most sensitive to sharp bass hits. _BeatPulse adds event-level impact. _SmoothedBass adds only a small continuous vibration.
            const double CameraShake = Clamp((_BassImpulse * 0.72) + (_BeatPulse * 0.48) + (_SmoothedBass * 0.10));

            constexpr double AttackResponse  = 34.; // Camera shake rises rapidly.
            constexpr double ReleaseResponse =  8.; // Camera shake fades more slowly:

            _CameraShake = ExponentialSmoothing(_CameraShake, CameraShake, deltaTime, (CameraShake > _CameraShake) ? AttackResponse : ReleaseResponse);
        }

        // Forward travel.
        {
            constexpr double ConstantSpeed = 0.080;

            const double OverallLoudness   = _SmoothedRMS  * 0.38;                 // Louder music produces generally faster travel. RMS modifies speed continuously rather than creating sharp bursts.
            const double SustainedBass     = _SmoothedBass * 0.62;                 // Bass-heavy passages push the camera forward more strongly.
            const double BeatEvent         = _BeatPulse    * 0.80 * _BeatResponse; // A detected beat creates a brief speed burst.
            const double BassTransient     = _BassResponse * 0.95;                 // Largest coefficient. A sharp kick creates the strongest forward acceleration.

            const double Forward = (ConstantSpeed + OverallLoudness + (SustainedBass * _BassResponse) + BeatEvent + (_BassImpulse * BassTransient)) * SpeedFactor;

            _TravelDistance += Forward * deltaTime;
        }

        // Tunnel rotation.
        {
            constexpr double ConstantSpeed = 0.045;

            const double MidContribution    = _SmoothedMid    * 0.16; // Strong rotation influence. Make vocals, guitars, synths, and melodic content affect the tunnel without producing the heavy physical impacts assigned to bass.
            const double TrebleContribution = _SmoothedTreble * 0.08; // Treble has a smaller effect, adding activity without causing excessive spinning.
            const double BassContribution   = _BassTwist      * 0.20; // Bass contributes through the specially shaped twist envelope rather than directly through raw bass energy. That gives it a punchy attack and slower rotational release.

            _TunnelRotation += (ConstantSpeed + MidContribution + TrebleContribution + BassContribution) * (_RotationDirection * deltaTime * SpeedFactor);

            _TunnelRotation = std::remainder(_TunnelRotation, TwoPi); // Angle wrapping
        }

        // Camera roll.
        {
            const double PrimaryRoll   = std::sin(_AnimationTime * 0.160) * 0.34;  // Frequency 0.160, amplitude 0.34
            const double SecondaryRoll = std::sin(_AnimationTime * 0.061) * 0.16;  // Frequency 0.061, amplitude 0.16
            const double BassScale     = 1.0 + (_SmoothedBass * 0.18);

            _CameraRoll = (PrimaryRoll + SecondaryRoll) * BassScale;
        }

        // Advance one shared cyclic phase rather than subtracting directly from every Z value. Reciprocal-depth mapping keeps projected ring spacing stable throughout the tunnel and across wrap-around.
        // Derive one physical-depth offset from the shared travel value. Base phases never change, so neighboring rings remain exactly one fixed Z interval apart at every frame and during wrap-around.
        {
            // Convert travel into a cyclic ring phase.
            _RingPhaseOffset = std::fmod(_TravelDistance / TunnelDepth, 1.0);

            if (_RingPhaseOffset < 0.0)
                _RingPhaseOffset += 1.0;
        }

        for (auto & Ring : _Rings)
        {
            Ring.PreviousZ = Ring.Z;

            const double visualPhase = Ring.Phase + _RingPhaseOffset;

            Ring.Z = PhaseToZ(visualPhase);

            // Assign the destination theme only when this ring wraps from the near plane back to the far plane. Existing rings retain the theme with which they entered the tunnel.
            if (Ring.Z > Ring.PreviousZ)
            {
                Ring.ThemeFrom  = _ThemeFrom;
                Ring.ThemeTo    = _ThemeTo;
                Ring.ThemeBlend = _ThemeBlend;
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _Rings.clear();

        // The longer depth range uses proportionally more rings, retaining a dense distant section while keeping constant physical spacing. High quality adds extra samples for smoother rail curvature.
        const size_t Count = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low:
                    return LowQualityRingCount;

                case EyeCandy::Quality::High:
                    return HighQualityRingCount;

                case EyeCandy::Quality::Medium:
                default:
                    return MediumQualityRingCount;
            }
        }();

        _Rings.resize(Count);

        _RingPhaseOffset = 0.;

        for (size_t i = 0; i < Count; ++i)
        {
            uint32_t State = Hash((uint32_t) i ^ 0xA53C9E17u);

            auto & Ring = _Rings[i];

            Ring.Seed        = State;
            Ring.Phase       = ((double) i + 0.5) / (double) Count;
            Ring.Z           = PhaseToZ(Ring.Phase);
            Ring.PreviousZ   = Ring.Z;
            Ring.ThemeFrom   = _ThemeFrom;
            Ring.ThemeTo     = _ThemeTo;
            Ring.ThemeBlend  = _ThemeBlend;
            Ring.RadiusScale = Lerp(0.985, 1.015, Random(State));
            Ring.Brightness  = Lerp(0.72, 1., Random(State));
            Ring.HueOffset   = RandomSigned(State);
        }
    }

    /// <summary>
    /// Render the background with a subtle shine in the left top corner.
    /// </summary>
    void effect_t::RenderBackground(ID2D1RenderTarget * renderTarget) noexcept
    {
        renderTarget->Clear(D2D1::ColorF(0.001f, 0.001f, 0.006f, 1.0f));

        const auto Opacity = (float) Clamp(0.76 + (_SmoothedRMS * 0.20) + (_SmoothedBass * 0.13));

        _BackgroundBrush->SetStartPoint(D2D1::Point2F(0, 0));
        _BackgroundBrush->SetEndPoint  (D2D1::Point2F(_Width, _Height));
        _BackgroundBrush->SetOpacity(Opacity);

        renderTarget->FillRectangle(_Rectangle, _BackgroundBrush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderTunnel(ID2D1RenderTarget * renderTarget)
    {
        if (_ProjectedRings.empty())
            return;

        std::vector<size_t> order(_ProjectedRings.size());

        std::iota(order.begin(), order.end(), size_t{ 0 });

        std::sort(order.begin(), order.end(), [this](size_t left, size_t right)
        {
            return _Rings[left].Z > _Rings[right].Z;
        });

        std::vector<projected_ring_t> ProjectedRings;

        ProjectedRings.reserve(_ProjectedRings.size());

        for (const size_t index : order)
            ProjectedRings.push_back(_ProjectedRings[index]);

        const auto Pulse = Clamp(_BeatPulse);

        // Draw every ring independently. A continuous alpha curve replaces the former three depth buckets, eliminating the visible brightness boundary halfway through the tunnel.
        // The exponent below brightens the far and middle sections earlier while preserving a transparent vanishing point.
        for (const projected_ring_t & pr : ProjectedRings)
        {
            if (!pr.Visible || pr.SideCount < 3)
                continue;

            const double Depth = Clamp(pr.Depth);

            const float LinearFade = SmoothStep(0.f, 1.f, (float) Depth);

            // Start at fully transparent alpha and use a sub-linear curve to reveal geometry gently but early. Applying SmoothStep a second time removes visible acceleration near both ends.
            const float EarlyFade = std::pow(LinearFade, 0.46f);
            const float LightFade = SmoothStep(0.f, 1.f, EarlyFade);

            const float OuterOpacity = std::clamp(LightFade * 0.18f + (float) Pulse * LightFade * 0.030f, 0.0f, 0.22f);
            const float CoreOpacity  = std::clamp(LightFade * 0.80f + (float) Pulse * LightFade * 0.085f, 0.0f, 0.90f);

            const float OuterWidth   = 1.25f + LightFade * 4.10f + (float) _BassExpansion * LightFade * 0.80f;
            const float CoreWidth    = 0.55f + LightFade * 0.92f + (float) Pulse          * LightFade * 0.18f;

            const color_theme_t & RingThemeFrom = GetTheme(pr.ThemeFrom);
            const color_theme_t & RingThemeTo   = GetTheme(pr.ThemeTo);

            const D2D1_COLOR_F RingColorFrom = GetThemeColor(RingThemeFrom, RingThemeFrom.RingHue, 0.0f, 0.90f, 1.0f, 1.0f);
            const D2D1_COLOR_F RingColorTo   = GetThemeColor(RingThemeTo,   RingThemeTo  .RingHue, 0.0f, 0.90f, 1.0f, 1.0f);
            const D2D1_COLOR_F TunnelColor   = LerpColor(RingColorFrom, RingColorTo, (float) pr.ThemeBlend);

            D2D1_COLOR_F outerColor = TunnelColor;
            outerColor.a *= OuterOpacity;

            D2D1_COLOR_F coreColor = TunnelColor;
            coreColor.a *= CoreOpacity;

            for (size_t side = 0; side < pr.SideCount; ++side)
            {
                const size_t next = (side + 1) % pr.SideCount;
                const D2D1_POINT_2F & from = pr.Points[side];
                const D2D1_POINT_2F & to = pr.Points[next];

                _RingBrush->SetColor(outerColor);

                renderTarget->DrawLine(from, to, _RingBrush.Get(), OuterWidth, _RoundStrokeStyle.Get());

                _RingBrush->SetColor(coreColor);

                renderTarget->DrawLine(from, to, _RingBrush.Get(), CoreWidth, _RoundStrokeStyle.Get());
            }
        }

        // Draw rails segment by segment using the same continuous fade.
        // Alpha starts close to transparent at the center and grows without a layer boundary. The sub-linear fade exponent prevents the middle of the tunnel from remaining excessively dark.
        if (ProjectedRings.size() >= 2)
        {
            const size_t SideCount = ProjectedRings.front().SideCount;

            for (size_t i = 0; i < SideCount; ++i)
            {
                for (size_t j = 1; j < ProjectedRings.size(); ++j)
                {
                    const projected_ring_t & FromRing = ProjectedRings[j - 1];
                    const projected_ring_t & ToRing   = ProjectedRings[j];

                    if (!FromRing.Visible || !ToRing.Visible || i >= FromRing.SideCount || i >= ToRing.SideCount)
                        continue;

                    const double Depth = (float) Clamp(0.5 * (FromRing.Depth + ToRing.Depth));

                    const float LinearFade = SmoothStep(0.0f, 1.0f, (float) Depth);
                    const float EarlyFade  = std::pow(LinearFade, 0.46f);
                    const float LightFade  = SmoothStep(0.0f, 1.0f, EarlyFade);

                    const float OuterOpacity = std::clamp(LightFade * 0.13f + (float) Pulse * LightFade * 0.018f, 0.0f, 0.17f);
                    const float CoreOpacity  = std::clamp(LightFade * 0.61f + (float) Pulse * LightFade * 0.060f, 0.0f, 0.70f);

                    const float OuterWidth   = 0.90f + LightFade * 3.25f + (float) _BassExpansion * LightFade * 0.65f;
                    const float CoreWidth    = 0.38f + LightFade * 1.15f + (float) Pulse          * LightFade * 0.24f;

                    // Resolve both endpoint snapshots and average them for the rail segment. This removes visible color steps at ring boundaries while preserving each ring's captured theme state.
                    const color_theme_t & FromThemeA = GetTheme(FromRing.ThemeFrom);
                    const color_theme_t & FromThemeB = GetTheme(FromRing.ThemeTo);

                    const D2D1_COLOR_F fromColor = LerpColor
                    (
                        GetThemeColor(FromThemeA, FromThemeA.RingHue, 0.0f, 0.90f, 1.0f, 1.0f),
                        GetThemeColor(FromThemeB, FromThemeB.RingHue, 0.0f, 0.90f, 1.0f, 1.0f),
                        (float) FromRing.ThemeBlend
                    );

                    const color_theme_t & ToThemeA = GetTheme(ToRing.ThemeFrom);
                    const color_theme_t & ToThemeB = GetTheme(ToRing.ThemeTo);

                    const D2D1_COLOR_F ToColor = LerpColor
                    (
                        GetThemeColor(ToThemeA, ToThemeA.RingHue, 0.0f, 0.90f, 1.0f, 1.0f),
                        GetThemeColor(ToThemeB, ToThemeB.RingHue, 0.0f, 0.90f, 1.0f, 1.0f),
                        (float) ToRing.ThemeBlend
                    );

                    const D2D1_COLOR_F TunnelColor = LerpColor(fromColor, ToColor, 0.5f);

                    D2D1_COLOR_F OuterColor = TunnelColor;

                    OuterColor.a *= OuterOpacity;

                    D2D1_COLOR_F CoreColor = TunnelColor;

                    CoreColor.a *= CoreOpacity;

                    const D2D1_POINT_2F & From = FromRing.Points[i];
                    const D2D1_POINT_2F & To   = ToRing.Points[i];

                    _RailBrush->SetColor(OuterColor);

                    renderTarget->DrawLine(From, To, _RailBrush.Get(), OuterWidth, _RoundStrokeStyle.Get());

                    _RailBrush->SetColor(CoreColor);

                    renderTarget->DrawLine(From, To, _RailBrush.Get(), CoreWidth, _RoundStrokeStyle.Get());
                }
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderCoreGlow(ID2D1RenderTarget * renderTarget) noexcept
    {
        const D2D1_POINT_2F Center = ProjectPoint(0., FarPlane, 0.);

        const float Dimension = std::min(_Width, _Height);

        const double AudioAmount = Clamp((_SmoothedBass * 0.58) + (_BassImpulse * 0.72) + (_BeatPulse * 0.16));

        // Keep the center glow broad and restrained. The gradient itself supplies the radial falloff; low brush opacity prevents a bright disc from filling the vanishing point.
        const auto OuterRadius = Dimension * (0.072f + (float) AudioAmount * 0.045f);

        {
            const auto Opacity = std::clamp(0.055f + (float) AudioAmount * 0.095f, 0.0f, 0.16f);

            _CoreBrush->SetCenter(Center);
            _CoreBrush->SetGradientOriginOffset(D2D1::Point2F(0.0f, 0.0f));
            _CoreBrush->SetRadiusX(OuterRadius);
            _CoreBrush->SetRadiusY(OuterRadius);
            _CoreBrush->SetOpacity(Opacity);

            renderTarget->FillEllipse(D2D1::Ellipse(Center, OuterRadius, OuterRadius), _CoreBrush.Get());
        }

        // A smaller second pass adds a faint focal point without obscuring the dark center opening.
        const float InnerRadius = OuterRadius * 0.42f;

        {
            _CoreBrush->SetRadiusX(InnerRadius);
            _CoreBrush->SetRadiusY(InnerRadius);
            _CoreBrush->SetOpacity(std::clamp(0.030f + (float) AudioAmount * 0.055f, 0.0f, 0.09f));

            renderTarget->FillEllipse(D2D1::Ellipse(Center, InnerRadius, InnerRadius), _CoreBrush.Get());
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderBeatFlash(ID2D1RenderTarget * renderTarget) noexcept
    {
        const auto Amount = (float) Clamp(_BeatPulse + _BassImpulse);

        if (Amount <= 0.003f)
                return;

        const auto Center  = ProjectPoint(0., FarPlane, 0.);
        const float Radius = std::min(_Width, _Height) * (0.055f + (Amount * 0.22f));

        _FlashBrush->SetCenter(Center);
        _FlashBrush->SetRadiusX(Radius); _FlashBrush->SetRadiusY(Radius);
        _FlashBrush->SetOpacity(Amount * 0.55f);

        renderTarget->FillEllipse(D2D1::Ellipse(Center, Radius, Radius), _FlashBrush.Get());
    }

    /// <summary>
    /// Projects the points of ring from 3D to 2D.
    /// </summary>
    effect_t::projected_ring_t effect_t::ProjectRing(const ring_t & ring, size_t index) const noexcept
    {
        projected_ring_t r {};

        r.SideCount   = (size_t) ((_Quality == EyeCandy::Quality::Low) ? 8 : (_Quality == EyeCandy::Quality::High) ? MaximumTunnelSides : 10);
        r.SourceIndex = index;
        r.ThemeFrom   = ring.ThemeFrom;
        r.ThemeTo     = ring.ThemeTo;
        r.ThemeBlend  = ring.ThemeBlend;
        r.Depth       = Clamp(1. - (ring.Z - NearPlane) / TunnelDepth);

        const double Near       = SmoothStep(0.25f, 1.0f, (float) r.Depth);
        const double Pulse      = (float) (1.0f + (_BassExpansion * 0.10f) + (_BassImpulse * Near * 0.11f) + (_BeatPulse * Near * 0.055f) + (_SmoothedBands[index % FrequencyBandCount] * 0.018f));
        const double DepthTwist = (float) (r.Depth * MaximumDepthTwist * _RotationDirection);
        const double Wave       = (float) (std::sin(ring.Z * 5.5f - (float) _AnimationTime * (0.42f + _BassTwist * 0.28f) * (float) _RotationDirection));

        const double Twist = (float) (_TunnelRotation + _CenterRotationSway + DepthTwist + Wave * MaximumBassTwist * Clamp(_BassTwist));

        for (size_t i = 0; i < r.SideCount; ++i)
            r.Points[i] = ProjectPoint(((double) i / (double) r.SideCount * TwoPi + Twist), ring.Z, ring.RadiusScale * Pulse);

        r.Brightness = ring.Brightness * Lerp(0.28f, 1.0f, SmoothStep(0.05f, 0.92f, (float) r.Depth));
        r.Visible    = true;

        return r;
    }

    /// <summary>
    /// Projects a point from 3D to 2D.
    /// </summary>
    D2D1_POINT_2F effect_t::ProjectPoint(double angle, double z, double radiusScale) const noexcept
    {
        z = std::clamp(z, NearPlane, FarPlane);

        const double Scale = std::min(_Width, _Height);
        const double Depth = Clamp(1. - (z - NearPlane) / TunnelDepth);

        const double NearWeight  = SmoothStep(0.0f, 1.0f, (float) Depth);
        const double CurveWeight = NearWeight * NearWeight;
        const double PathTime    = _AnimationTime * 0.20f + _TravelDistance * 0.72f;

        const double BassDrive = std::clamp(_SmoothedBass * 0.72 + _BassImpulse * 0.78 + _BeatPulse * 0.22, 0., 1.55);
        const double MidDrive  = Clamp(_SmoothedMid  * 0.82 + _SmoothedTreble * 0.14); 

        // Phase-modulated, incommensurate sinusoidal components create an organic path that continuously changes shape without jumps or a visibly repeating circular orbit.
        const double SlowPhase   = PathTime * 0.31f + std::sin(PathTime * 0.071f + 1.40f) * 0.55f;
        const double MediumPhase = PathTime * 0.83f + std::sin(PathTime * 0.137f + 3.10f) * 0.42f;
        const double FastPhase   = PathTime * 1.43f + std::sin(PathTime * 0.223f + 5.00f) * 0.24f;

        const double SwayX = std::sin(MediumPhase + 0.35f)         * 0.48f + std::sin(FastPhase + 2.10f)         * 0.24f + std::sin(SlowPhase + 4.20f)         * 0.20f + std::sin(MediumPhase * 0.53f + FastPhase * 0.17f) * 0.08f;
        const double SwayY = std::cos(MediumPhase * 0.91f + 1.05f) * 0.45f + std::sin(FastPhase * 0.87f + 3.35f) * 0.25f + std::cos(SlowPhase * 1.11f + 5.10f) * 0.21f + std::cos(MediumPhase * 0.29f - FastPhase * 0.23f) * 0.09f;

        const double GlobalAmplitude = Scale * (0.052f + BassDrive * 0.052f + MidDrive * 0.017f);

        const double KickScale = Scale * 0.155f;

        const double GlobalOffsetX = SwayX * GlobalAmplitude + _CenterKickX * KickScale;
        const double GlobalOffsetY = SwayY * GlobalAmplitude + _CenterKickY * KickScale;

        const double PrimaryPhase   = MediumPhase *  0.91f + z * (3.05f + BassDrive * 1.35f) + _BassTwist * 0.35f * _RotationDirection;
        const double SecondaryPhase = FastPhase   * -0.47f + z * (5.45f + MidDrive  * 1.65f) + 1.21f;

        const double WindingX = Scale * (0.010f + BassDrive * 0.045f + MidDrive * 0.014f);
        const double WindingY = Scale * (0.009f + BassDrive * 0.039f + MidDrive * 0.016f);

        const double TrebleRipple = Scale * _SmoothedTreble * 0.0045f * CurveWeight;

        const double DesiredOffsetX = GlobalOffsetX + std::sin(PrimaryPhase)                 * WindingX * CurveWeight + std::sin(SecondaryPhase)         * Scale * MidDrive * 0.020f * CurveWeight + std::sin(FastPhase + z * 9.0f)         * TrebleRipple;
        const double DesiredOffsetY = GlobalOffsetY + std::cos(PrimaryPhase * 0.91f + 0.31f) * WindingY * CurveWeight + std::cos(SecondaryPhase * 1.06f) * Scale * MidDrive * 0.018f * CurveWeight + std::cos(FastPhase * 0.89f + z * 8.2f) * TrebleRipple;

        // Soft-limit the center before it reaches the viewport edges. Tanh preserves small movement almost linearly and progressively compresses only large excursions, avoiding a hard bounce.
        const double EdgeMargin = Scale * 0.17f;

        const double LimitX = std::max(Scale * 0.10f, _CenterX - EdgeMargin);
        const double LimitY = std::max(Scale * 0.10f, _CenterY- EdgeMargin);

        const double LimitedOffsetX = LimitX * std::tanh(DesiredOffsetX / LimitX);
        const double LimitedOffsetY = LimitY * std::tanh(DesiredOffsetY / LimitY);

        const double CenterX = _CenterX  + LimitedOffsetX;
        const double CenterY = _CenterY + LimitedOffsetY;

        const double ProjectedRadius = Scale * 0.158f * radiusScale / z;

        return
        {
            (float) (CenterX + std::cos((double) angle) * ProjectedRadius),
            (float) (CenterY + std::sin((double) angle) * ProjectedRadius)
        };
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::CreateRingGeometry(ID2D1Factory * factory, const std::vector<projected_ring_t> & rings, std::array<ComPtr<ID2D1PathGeometry>, GlowLayerCount> & geometries)
    {
        std::array<ComPtr<ID2D1GeometrySink>, GlowLayerCount> Sinks;

        for (size_t i = 0; i < GlowLayerCount; ++i)
        {
            if (SUCCEEDED(factory->CreatePathGeometry(&geometries[i])))
                geometries[i]->Open(&Sinks[i]);
        }

        for (const auto & Rings : rings)
        {
            const size_t Layer = (size_t) ((Rings.Depth > 0.66f) ? 2 : (Rings.Depth > 0.28f) ? 1 : 0);

            auto & Sink = Sinks[Layer];

            if (!Sink)
                continue;

            Sink->BeginFigure(Rings.Points[0], D2D1_FIGURE_BEGIN_HOLLOW);

            for (size_t i = 1; i < Rings.SideCount; ++i)
                Sink->AddLine(Rings.Points[i]);

            Sink->AddLine(Rings.Points[0]);

            Sink->EndFigure(D2D1_FIGURE_END_OPEN);
        }

        for (size_t i = 0; i < GlowLayerCount; ++i)
        {
            if (Sinks[i] && FAILED(Sinks[i]->Close()))
                geometries[i].Reset();
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::CreateRailGeometry(ID2D1Factory * factory, const std::vector<projected_ring_t> & rings, std::array<ComPtr<ID2D1PathGeometry>, GlowLayerCount> & geometries)
    {
        if (rings.size() < 2)
            return;

        const size_t SideCount = rings.front().SideCount;

        std::array<ComPtr<ID2D1GeometrySink>, GlowLayerCount> Sinks;

        for (size_t i = 0; i < GlowLayerCount; ++i)
        {
            if (SUCCEEDED(factory->CreatePathGeometry(&geometries[i])))
                geometries[i]->Open(&Sinks[i]);
        }

        for (size_t i = 0; i < SideCount; ++i)
        {
            size_t ActiveLayer = GlowLayerCount;

            bool IsOpen = false;

            for (const auto & pr : rings)
            {
                const size_t Layer = (size_t)((pr.Depth > 0.66f) ? 2 : (pr.Depth > 0.28f) ? 1 : 0);

                if (IsOpen && (Layer != ActiveLayer))
                {
                    Sinks[ActiveLayer]->EndFigure(D2D1_FIGURE_END_OPEN);

                    IsOpen = false;
                }

                if (!Sinks[Layer])
                    continue;

                const auto pt = pr.Points[i];

                if (!IsOpen)
                {
                    Sinks[Layer]->BeginFigure(pt, D2D1_FIGURE_BEGIN_HOLLOW);

                    ActiveLayer = Layer;
                    IsOpen = true;
                }
                else
                    Sinks[Layer]->AddLine(D2D1::Point2F(pt.x, pt.y));
            }

            if (IsOpen)
                Sinks[ActiveLayer]->EndFigure(D2D1_FIGURE_END_OPEN);
        }

        for (size_t i = 0; i < GlowLayerCount; ++i)
        {
            if (Sinks[i] && FAILED(Sinks[i]->Close()))
                geometries[i].Reset();
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
                _Workers.emplace_back(&effect_t::Worker, this);
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
            {
                std::unique_lock lock(_WorkerMutex);

                _WorkerCondition.wait(lock, [this, &Generation]() noexcept
                {
                    return _IsStopping || _Generation != Generation;
                });

                if (_IsStopping)
                    return;

                Generation = _Generation;
            }

            for (;;)
            {
                const size_t RingIndex = _NextRing.fetch_add(1, std::memory_order_relaxed);

                if (RingIndex >= _Rings.size())
                    break;

                _ProjectedRings[RingIndex] = ProjectRing(_Rings[RingIndex], RingIndex);
            }

            {
                const std::lock_guard Lock(_WorkerMutex);

                if (--_PendingWorkers == 0)
                    _CompletionCondition.notify_one();
            }
        }
    }

    /// <summary>
    /// Generates the work for the worker threads.
    /// </summary>
    void effect_t::GenerateWork() noexcept
    {
        if (_Rings.empty() || _Workers.empty())
        {
            _ProjectedRings.clear();

            return;
        }

        _ProjectedRings.resize(_Rings.size());

        {
            const std::lock_guard Lock(_WorkerMutex);

            _NextRing.store(0, std::memory_order_relaxed);

            _PendingWorkers = _Workers.size();
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
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr =__super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if (!_ThemeResourcesDirty && _RingBrush && _RailBrush && _CoreBrush && _FlashBrush && _RoundStrokeStyle)
            return S_OK;

        hr = renderTarget->CreateSolidColorBrush(GetThemeColor(ThemeRole::Ring, 0, 0.9f, 1, 1), _RingBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(GetThemeColor(ThemeRole::Rail, 0, 0.9f, 1, 1), _RailBrush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        {
            const D2D1_GRADIENT_STOP Core[] =
            {
                { 0.00f, GetThemeColor(ThemeRole::Core, 0.00f, 0.08f, 1.00f, 0.98f) },
                { 0.12f, GetThemeColor(ThemeRole::Core, 0.08f, 0.72f, 1.00f, 0.82f) },
                { 0.42f, GetThemeColor(ThemeRole::Core,-0.24f, 0.94f, 0.88f, 0.34f) },
                { 1.00f, GetThemeColor(ThemeRole::Core, 0.34f, 0.95f, 0.20f, 0.00f) }
            };

            hr = renderTarget->CreateGradientStopCollection(Core, _countof(Core), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, _CoreStops.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;

            hr = renderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(_CenterX, _CenterY), D2D1::Point2F(0, 0), 100, 100), _CoreStops.Get(), _CoreBrush.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            const D2D1_GRADIENT_STOP Flash[] =
            {
                { 0.00f, GetThemeColor(ThemeRole::Flash, 0.00f, 0.00f, 1.00f, 0.98f) },
                { 0.18f, GetThemeColor(ThemeRole::Flash, 0.08f, 0.60f, 1.00f, 0.78f) },
                { 0.52f, GetThemeColor(ThemeRole::Flash,-0.18f, 0.95f, 0.88f, 0.26f) },
                { 1.00f, GetThemeColor(ThemeRole::Flash, 0.28f, 0.92f, 0.18f, 0.00f) }
            };

            hr = renderTarget->CreateGradientStopCollection(Flash, _countof(Flash), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, _FlashStops.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;

            hr = renderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(_CenterX, _CenterY), D2D1::Point2F(0, 0), 100, 100), _FlashStops.Get(), _FlashBrush.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f);

            hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _RoundStrokeStyle.ReleaseAndGetAddressOf());
        }

        _ThemeResourcesDirty = false;

        _RenderTarget = renderTarget;

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _RenderTarget = nullptr;

        _RingBrush.Reset();
        _RailBrush.Reset();

        _CoreStops.Reset();
        _CoreBrush.Reset();
        _FlashStops.Reset();
        _FlashBrush.Reset();
        _RoundStrokeStyle.Reset();

        __super::DeleteDeviceDependentResources();
    }
}


/** $VER: Swarm.cpp (2026.11.07) */

#include <pch.h>

#include "Swarm.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Swarm
{
    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Swarm, {} quality, {} speed, {} brightness", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        const size_t Count = (_Quality == Quality::Low) ? 420u : (_Quality == Quality::High) ? 1500u : 850u;

        _Agents.resize(Count);
        _Snapshots.resize(Count);

        for (size_t i = 0; i < Count; ++i)
            RespawnAgent(_Agents[i], i);
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

        {
            _BackBufferRenderTarget->BeginDraw();

            Fade(_BackBufferRenderTarget.Get(), deltaTime);

            DrawBackgroundGradient(_BackBufferRenderTarget.Get(), deltaTime);

            if (_HasAudio)
                RenderSwarm(_BackBufferRenderTarget.Get());
            else
                RenderIdleSwarm(_BackBufferRenderTarget.Get());

            if (FAILED(_BackBufferRenderTarget->EndDraw()))
            {
                DeleteDeviceDependentResources();

                return;
            }
        }

        {
            renderTarget->DrawBitmap(_BackBuffer.Get(), _ViewPort);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _HasAudio = false;

        _Peak          = 0.;
        _RMS           = 0.;
        _SpectralFlux  = 0.;
        _StereoBalance = 0.;

        if (chunk.is_empty())
        {
            for (auto & Band : _Bands)
                Band.Energy = 0.;

            return;
        }

        const audio_sample * const Frames = chunk.get_data();

        const auto FrameCount   = chunk.get_sample_count();
        const auto ChannelCount = chunk.get_channel_count();
        const auto SampleRate   = (double) chunk.get_sample_rate();

        // Analyze only the most recent frames of the chunk.
        const auto MaxFrames = std::min<size_t>(FrameCount, 2048); // 2048 frames @ 44100 Hz ~ 46 ms

        if (MaxFrames < 32)
            return;

        if (_Samples.size() < MaxFrames)
            _Samples.resize(MaxFrames);

        auto Samples = std::span<double>(_Samples.data(), MaxFrames);

        // Initialize the energy accumulators.
        double Sum  = 0.;           // Full-audio band

        double LeftPower  = 0.;
        double RightPower = 0.;

        // Process all frames.
        {
            for (size_t i = 0; i < MaxFrames; ++i)
            {
                const audio_sample * Frame = Frames + (i * ChannelCount);

                // Convert the frame to mono.
                double Mono = 0.;
                {
                    for (unsigned c = 0; c < ChannelCount; ++c)
                        Mono += Frame[c];

                    Mono = ClampAudio(Mono / (double) ChannelCount);
                }

                // Apply a Hann window.
                {
                    const double Window = 0.5 - (std::cos(TwoPi * (double) i / ((double) MaxFrames - 1.)) * 0.5);

                    Samples[i] = Mono * Window;
                }

                // Calculate peak level.
                _Peak = std::max(_Peak, std::abs(Mono));

                // Accumulate the full-band RMS energy.
                Sum += Mono * Mono;

                // Accumulate the stereo power.
                LeftPower += Frame[0] * Frame[0];

                const double Right = Frame[ChannelCount > 1 ? 1 : 0];

                RightPower += Right * Right;
            }
        }

        _RMS = std::sqrt(Sum / (double) MaxFrames);

        // Detect silence.
        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);

        if (!_HasAudio)
            return;

        // Calculate the stereo balance.
        {
            const double StereoPower = LeftPower + RightPower;

            _StereoBalance = (StereoPower > 1.e-12) ? std::clamp((RightPower - LeftPower) / StereoPower, -1., 1.) : 0.;
        }

        // Perform a spectrum analysis of logarithmically spaced frequency bands.
        {
            const double NyquistFrequency = SampleRate * 0.5;
            const double MaximumFrequency = std::min(HiFrequency, NyquistFrequency);

            constexpr double TuningConstant = 80.;
            const double LogarithmicScale  = 1. / std::log1p(TuningConstant);

            double MaximumEnergy = 0.;

            for (size_t i = 0; i < BandCount; ++i)
            {
                const double BandAmount = (double) i * InverseBandCount; // [0, 1]

                const double CenterFrequency = LoFrequency * std::pow(MaximumFrequency / LoFrequency, BandAmount);

                double Power = 0.;

                // Sample three frequencies per band: roughly one whole tone below the center, the center frequency and one whole tone above the center (std::pow(2, -1/6), 1, std::pow(2, +1/6))
                for (double Ratio : { .890898718, 1., 1.122462048 })
                {
                    const double Frequency = std::clamp(CenterFrequency * Ratio, LoFrequency, NyquistFrequency);

                    {
                        // Configure the Goertzel detector.
                        const double Omega       = TwoPi * Frequency / SampleRate;
                        const double Coefficient = 2. * std::cos(Omega);

                        double q1 = 0., q2 = 0.;

                        // Run the Goertzel recurrence over the windowed signal.
                        for (double Sample : Samples)
                        {
                            const double q0 = Sample + Coefficient * q1 - q2;

                            q2 = q1;
                            q1 = q0;
                        }

                        // Convert the detector state to power.
                        Power += std::max(0., q2 * q2 + q1 * q1 - Coefficient * q1 * q2);
                    }
                }

                const double Magnitude = std::sqrt(Power / 3.) / (double) MaxFrames;

                // Compress (logarithmic) and shape the band energy. Perform high-frequency weighting: 0.90 for the lowest band, 1.50 for the highest band.
                _Bands[i].Energy = Clamp(std::log1p(Magnitude * TuningConstant) * LogarithmicScale * Lerp(0.90, 1.50, BandAmount));

                // Track the highest frequency band energy.
                MaximumEnergy = std::max(MaximumEnergy, _Bands[i].Energy);
            }

            // Normalize the spectrum.
            if (MaximumEnergy > 1.0e-5)
            {
                const auto Normalization = 1. / std::max(0.30, MaximumEnergy);

                for (auto & Band : _Bands)
                    Band.Energy = Clamp(Band.Energy * Normalization);
            }
        }

        // Calculate the half-wave rectified spectral flux. It responds when spectral energy increases such as during drum hits, note attacks, sudden treble bursts and new musical events.
        {
            _SpectralFlux = 0.;

            for (auto & Band : _Bands)
            {
                _SpectralFlux += std::max(0., Band.Energy - Band.PreviousEnergy);

                Band.PreviousEnergy = Band.Energy; // Remember the band energy for the next chunk.
            }

            _SpectralFlux = Clamp(_SpectralFlux / (double) BandCount * 5.5); // Boost the spectral flux artificially to make it react more strongly.
        }
    }

    /// <summary>
    /// Prepares the visual parameters.
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        if (_Agents.empty())
            Reset();

        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case Speed::Slow:
                    return 0.55;

                default:
                case Speed::Normal:
                    return 1.00;

                case Speed::Fast:
                    return 1.75;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;

        {
            _SmoothedPeak          = ExponentialSmoothing(_SmoothedPeak,          _Peak,          deltaTime, (_Peak         > _SmoothedPeak) ? 24. : 5.);
            _SmoothedRMS           = ExponentialSmoothing(_SmoothedRMS,           _RMS,           deltaTime, (_RMS          > _SmoothedRMS)  ? 20. : 4.);

            _SmoothedSpectralFlux  = ExponentialSmoothing(_SmoothedSpectralFlux,  _SpectralFlux,  deltaTime, (_SpectralFlux > _SmoothedSpectralFlux) ? 36. : 6.);
            _SmoothedStereoBalance = ExponentialSmoothing(_SmoothedStereoBalance, _StereoBalance, deltaTime, 8.);
        }

        _PreviousBeatPulse = _BeatPulse;

        _BeatPulse      = std::max(_BeatPulse * std::exp(-deltaTime * 7.2), _SpectralFlux);
        _BeatCooldown   = std::max(0., _BeatCooldown - deltaTime);

        {
            const double BeatEdge = std::max(0., _BeatPulse - _PreviousBeatPulse);

            if (BeatEdge > .10 && _BeatCooldown <= 0.)
            {
                _PreviousPaletteIndex = _PaletteIndex;

                // Deliberately jump across the wheel instead of stepping to a neighboring palette.
                _PaletteIndex   = (_PaletteIndex + 2 + (size_t)(_BassLevel * 3.5)) % 6;
                _PaletteBlend   = 0.;
                _BeatCooldown   = .12;
                _BeatColorPhase = Fraction(_BeatColorPhase + .16 + BeatEdge * .30);
            }
        }

        _PaletteBlend   = std::min(1., _PaletteBlend + deltaTime * (7.5 + (_BeatPulse * 10.)));

        _BeatColorPhase = Fraction(_BeatColorPhase + deltaTime * (.018 + (_SmoothedRMS * .045)));
        _GradientPhase  = Fraction(_GradientPhase  + deltaTime * (.035 + (_BeatPulse   * .120)));

        {
            for (auto & Band : _Bands)
                Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, Band.Energy > Band.SmoothedEnergy ? 22. : 4.5);

            _BassLevel   = (_Bands[0].SmoothedEnergy + _Bands[1].SmoothedEnergy + _Bands[ 2].SmoothedEnergy + _Bands[ 3].SmoothedEnergy) * .25;
            _MidLevel    = (_Bands[4].SmoothedEnergy + _Bands[5].SmoothedEnergy + _Bands[ 6].SmoothedEnergy + _Bands[ 7].SmoothedEnergy) * .25;
            _TrebleLevel = (_Bands[8].SmoothedEnergy + _Bands[9].SmoothedEnergy + _Bands[10].SmoothedEnergy + _Bands[11].SmoothedEnergy) * .25;
        }

        {
            _Snapshots = _Agents;

            for (size_t i = 0; i < _Agents.size(); ++i)
                UpdateAgent(i, deltaTime * SpeedFactor);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateAgent(size_t index, double deltaTime) noexcept
    {
        agent_t & Agent = _Agents[index];

        const agent_t & Snapshot = _Snapshots[index];

        Agent.Previous = Snapshot.Position;
        Agent.Position = Snapshot.Position;
        Agent.Velocity = Snapshot.Velocity;

        const double Dimension  = std::max(1.f, std::min(_Width, _Height));
        const double Perception = Dimension * (.055 + (_MidLevel * .035));

        D2D1_POINT_2F Center {}, Alignment {}, Separation {};

        size_t Neighbors = 0;

        uint32_t State = Hash((uint32_t) index ^ Snapshot.Seed ^ 0xA511E9B3u);

        for (size_t n = 0; n < std::min(NeighborSamples, _Snapshots.size() - 1); ++n)
        {
            State = Hash(State + (uint32_t) n * 0x9E3779B9u);

            const size_t j = (index + 1 + State % (_Snapshots.size() - 1)) % _Snapshots.size();

            {
                const auto & Snapshot2 = _Snapshots[j];

                const double dx = Snapshot2.Position.x - Snapshot2.Position.x;
                const double dy = Snapshot2.Position.y - Snapshot2.Position.y;

                const double Distance = Length(dx, dy);

                if (Distance < 1.e-3 || Distance > Perception)
                    continue;

                Center.x += Snapshot2.Position.x;
                Center.y += Snapshot2.Position.y;

                Alignment.x += Snapshot2.Velocity.x;
                Alignment.y += Snapshot2.Velocity.y;

                if (Distance < Perception * .38)
                {
                    const double k = 1. - Distance / (Perception * .38);

                    Separation.x -= (float) (dx / Distance * k);
                    Separation.y -= (float) (dy / Distance * k);
                }
            }

            ++Neighbors;
        }

        double ax = 0., ay = 0.;

        if (Neighbors != 0)
        {
            const double Inv = 1. / (double) Neighbors;

            ax += (Center.x * Inv - Snapshot.Position.x) * (.42 + _MidLevel * .72);
            ay += (Center.y * Inv - Snapshot.Position.y) * (.42 + _MidLevel * .72);

            ax += (Alignment.x * Inv - Snapshot.Velocity.x) * (.75 + _TrebleLevel * 1.25);
            ay += (Alignment.y * Inv - Snapshot.Velocity.y) * (.75 + _TrebleLevel * 1.25);

            ax += Separation.x * Dimension * (1.8 + _BassLevel * 3.2);
            ay += Separation.y * Dimension * (1.8 + _BassLevel * 3.2);
        }

        const double BandAmount = (double) Snapshot.Band  * InverseBandCount; // [0, 1]
        const double Energy     = _Bands[Snapshot.Band].SmoothedEnergy;
        const double BalanceX   = _CenterX + (_SmoothedStereoBalance * _Width * .29);
        const double LaneY      = _CenterY + std::sin(Snapshot.Phase + _AnimationTime * (.25 + BandAmount)) * Dimension * (.08 + BandAmount * .18);

        ax += (BalanceX - Snapshot.Position.x) * (.32 + _BassLevel * .55);
        ay += (LaneY    - Snapshot.Position.y) * (.20 + _MidLevel  * .42);

        const double Heading      = std::atan2(Snapshot.Velocity.y, Snapshot.Velocity.x);
        const double ImpulseAngle = Heading + ((Hash(Snapshot.Seed) & 1u) ? 1. : -1.) * (1.1 + BandAmount * .9);
        const double Impulse      = Dimension * _BeatPulse * (1.2 + (Energy * 2.4));

        ax += std::cos(ImpulseAngle) * Impulse;
        ay += std::sin(ImpulseAngle) * Impulse;

        const double Noise = Snapshot.Phase + _AnimationTime * (1.2 + BandAmount * 2.8);

        ax += std::cos(Noise +        Snapshot.Position.y * .008) * Dimension * (.035 + _TrebleLevel * .08);
        ay += std::sin(Noise * 1.17 + Snapshot.Position.x * .007) * Dimension * (.035 + _TrebleLevel * .08);

        Agent.Velocity.x += (float) (ax * deltaTime);
        Agent.Velocity.y += (float) (ay * deltaTime);

        const double Drag = std::exp(-deltaTime * (1.05 - (_SmoothedRMS * .25)));

        Agent.Velocity.x *= (float) Drag;
        Agent.Velocity.y *= (float) Drag;

        const double MaxSpeed     = Dimension * (.34 + (_SmoothedRMS * .42) + (_BeatPulse * .28));
        const double CurrentSpeed = Length(Agent.Velocity.x, Agent.Velocity.y);

        if (CurrentSpeed > MaxSpeed)
        {
            const double Scale = MaxSpeed / CurrentSpeed;

            Agent.Velocity.x *= (float) Scale;
            Agent.Velocity.y *= (float) Scale;
        }

        Agent.Position.x += (float) (Agent.Velocity.x * deltaTime);
        Agent.Position.y += (float) (Agent.Velocity.y * deltaTime);

        const float Margin = (float) (Dimension * .08);

        bool Wrapped = false;

        if (Agent.Position.x < -Margin)
        {
            Agent.Position.x = _Width + Margin;
            Wrapped = true;
        }
        else
        if (Agent.Position.x > _Width + Margin)
        {
            Agent.Position.x = -Margin;
            Wrapped = true;
        }

        if (Agent.Position.y < -Margin)
        {
            Agent.Position.y = _Height + Margin;
            Wrapped = true;
        }
        else
        if (Agent.Position.y > _Height + Margin)
        {
            Agent.Position.y = -Margin;
            Wrapped = true;
        }

        // Wrapping teleports the agent to the opposite edge. Starting the trail there prevents a spurious full-height or full-width line.
        if (Wrapped)
            Agent.Previous = Agent.Position;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RespawnAgent(agent_t & agent, size_t index) noexcept
    {
        uint32_t State = Hash((uint32_t) index * 0x9E3779B9u ^ 0xB5297A4Du);

        agent.Seed     = State;
        agent.Band     = index % BandCount;
        agent.Position =
        {
            (float) Random(State) * _Width,
            (float) Random(State) * _Height
        };
        agent.Previous = agent.Position;

        const double Angle     = Random(State) * TwoPi;
        const double Magnitude = std::min(_Width, _Height) * Lerp(.04, .12, Random(State));

        agent.Velocity   =
        {
            (float) (std::cos(Angle) * Magnitude),
            (float) (std::sin(Angle) * Magnitude)
        };

        agent.Phase      = Random(State) * TwoPi;
        agent.Size       = Lerp(.7, 1.45, Random(State));
        agent.Brightness = Lerp(.75, 1.15, Random(State));
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawBackgroundGradient(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        constexpr int Bands = 48;

        const double Beat  = Clamp(_BeatPulse);
        const double Drift = _GradientPhase * TwoPi;

        const double PaletteHue = GetPaletteHue(.12, _PaletteBlend);

        // Animated vertical color field, blended lightly so persistent trails survive.
        for (int i = 0; i < Bands; ++i)
        {
            const double t = (double) i / (Bands - 1);

            const double centerGlow = 1. - std::abs(t * 2. - 1.);
            const double waveA      = .5 + .5 * std::sin(TwoPi * t * 1.4 + Drift);
            const double waveB      = .5 + .5 * std::cos(TwoPi * t * 2.2 - Drift * .63);
            const double hue        = Fraction(PaletteHue + t * .12 + waveB * .025);
            const double value      = .008 + centerGlow * .024 + waveA * .018 + Beat * centerGlow * .045;

            const auto Color = HSVToColor((float) hue, .92f, (float) std::clamp(value, 0., .085));

            SetBrush(Color, (float) std::clamp(deltaTime * 5.2, .025, .16));

            const float y0 = (float)((float) i * _Height / Bands);

            renderTarget->FillRectangle(D2D1::RectF(0.f, y0, _Width, y0 + (_Height / Bands) + 1.f), _Brush.Get());
        }

        // Moving radial blooms make the gradient breathe, pan, and flash with each beat.
        const float Span = std::min(_Width, _Height);

        for (int Lobe = 0; Lobe < 3; ++Lobe)
        {
            const double Phase = Drift + Lobe * TwoPi / 3.;

            const D2D1_POINT_2F center
            {
                (float) (_CenterX + std::sin(Phase * (1.   + Lobe * .13) + (_SmoothedStereoBalance * 1.8)) * _Width * (.24 + Beat * .15)),
                (float) (_CenterY + std::cos(Phase * ( .73 + Lobe * .09))                                  * _Height * .24)
            };

            const float Radius = Span * (float)(.24 + Lobe * .065 + Beat * .28);

            const auto Color = HSVToColor((float)Fraction(PaletteHue + Lobe * .115 + _BeatColorPhase * .18), .98f, 1.f);

            SetBrush(Color, (float) (.010 + (Beat * .018)));

            renderTarget->FillEllipse(D2D1::Ellipse(center, Radius, Radius), _Brush.Get());

            SetBrush(Color, (float) (.012 + (Beat * .026)));

            renderTarget->FillEllipse(D2D1::Ellipse(center, Radius * .48f, Radius * .48f), _Brush.Get());
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Persistence = .88 + (_SmoothedRMS * .055) + (_BeatPulse * .02);
        const double Decay       = std::pow(std::clamp(Persistence, .78, .965), deltaTime * 60.);

        _Brush->SetColor(D2D1::ColorF(0.f, 0.f, .008f, (float) (1. - Decay)));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_ViewPort, _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderSwarm(ID2D1RenderTarget * renderTarget) noexcept
    {
        const double Side = std::min(_Width, _Height);

        for (const auto & Agent : _Agents)
        {
            const double Speed     = Length(Agent.Velocity.x, Agent.Velocity.y);
            const double Energy    = _Bands[Agent.Band].SmoothedEnergy;
            const double Intensity = Agent.Brightness * (.58 + (Energy * 1.05) + (_SmoothedRMS * .78) + (_BeatPulse * .35)) * std::clamp(Speed / (Side * .20), .58, 1.45);

            const auto Color = Colorize(Agent.Band, Energy, Fraction(Agent.Phase / TwoPi + _AnimationTime * .025));

            const double Trail = std::clamp(.45 + (_SmoothedRMS * 1.05) + (_BeatPulse * .8), .45, 1.9);

            const D2D1_POINT_2F Tail
            {
                (float) (Agent.Position.x - (Agent.Position.x - Agent.Previous.x) * Trail),
                (float) (Agent.Position.y - (Agent.Position.y - Agent.Previous.y) * Trail)
            };

            DrawNeonLine (renderTarget, Tail, Agent.Position, Color, (float) Intensity,                        (float) (Agent.Size * (1.25 + Energy * 1.9)));
            DrawNeonPoint(renderTarget,       Agent.Position, Color, (float)(Intensity * (.45 + Energy * .3)), (float) (Agent.Size * (1.70 + Energy * 2.6)));
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderIdleSwarm(ID2D1RenderTarget * renderTarget) noexcept
    {
        const double Side = std::min(_Width, _Height);

        for (size_t i = 0; i < 84; ++i)
        {
            const double Angle  = (double) i / 84.;
            const double Phase  = Angle * TwoPi + (_AnimationTime * (.17 + Angle * .23));
            const double Radius = Side * (.10 + (.27 * Fraction(Angle * 7.)));

            const D2D1_POINT_2F p
            {
                (float) (_CenterX + std::cos(Phase * 2.1) * Radius),
                (float) (_CenterY + std::sin(Phase * 1.7) * Radius * .62)
            };

            const D2D1_POINT_2F q
            {
                (float) (p.x - std::cos(Phase) * 3.2),
                (float) (p.y - std::sin(Phase) * 3.2)
            };

            DrawNeonLine(renderTarget, q, p, HSVToColor((float)Fraction(.48 + Angle * .50), 1.f, 1.f), .32f, 1.35f);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::Colorize(size_t band, double energy, double phase) const noexcept
    {
        const double BandAmount = (double) band * InverseBandCount; // [0, 1]
        const double Position   = Fraction((BandAmount * .92) + (phase * .08) + (_SmoothedStereoBalance * .035));

        const float Hue = (float) GetPaletteHue(Position, _PaletteBlend);

        return HSVToColor(Hue, 1.f, (float) Clamp(.96 + (energy * .08) + (_BeatPulse * .16)));
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::GetPaletteHue(double position, double blend) const noexcept
    {
        // Six high-contrast neon palettes: cyber, ultraviolet, laser, plasma, arctic, and toxic.
        static constexpr double Base[] = { .50, .84, .04, .69, .33, .93 };
        static constexpr double Span[] = { .38, .18, .27, .24, .34, .14 };

        const auto hueFor = [position](size_t index) noexcept
        {
            return Fraction(Base[index] + position * Span[index]);
        };

        const double From = hueFor(_PreviousPaletteIndex % 6);
        const double To   = hueFor(_PaletteIndex % 6);

        double Delta = To - From;

        if (Delta >  .5) Delta -= 1.;
        if (Delta < -.5) Delta += 1.;

        const double Eased = blend * blend * (3. - 2. * blend);

        return Fraction(From + Delta * Eased + _BeatColorPhase);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawNeonLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, const D2D1_COLOR_F & color, float intensity, float width) noexcept
    {
        const float q = (_Quality == Quality::High) ? 1.35f : (_Quality == Quality::Low) ? .8f : 1.f;

        DrawLine(renderTarget, from, to, color, intensity * .055f, width * 9.0f * q);
        DrawLine(renderTarget, from, to, color, intensity * .120f, width * 5.5f * q);
        DrawLine(renderTarget, from, to, color, intensity * .300f, width * 2.8f * q);
        DrawLine(renderTarget, from, to, color, intensity * .780f, width * 1.35f);

        SetBrush(White, Clamp(intensity * .72f), 1.f);

        renderTarget->DrawLine(from, to, _Brush.Get(), std::max(.65f, width * .34f), _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawNeonPoint(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F pt, const D2D1_COLOR_F & color, float intensity, float radius) noexcept
    {
        if (radius <= 0.f || intensity <= .001f)
            return;

        SetBrush(color, intensity * .045f, 1.35f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius * 4.2f,  radius * 4.20f), _Brush.Get());
        SetBrush(color, intensity * .100f, 1.25f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius * 2.7f,  radius * 2.70f), _Brush.Get());
        SetBrush(color, intensity * .270f, 1.15f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius * 1.55f, radius * 1.55f), _Brush.Get());
        SetBrush(color, intensity * .880f, 1.05f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius *  .62f, radius *  .62f), _Brush.Get());

        SetBrush(White, intensity, 1.f);

        renderTarget->FillEllipse(D2D1::Ellipse(pt, std::max(.8f, radius * .22f), std::max(.8f, radius * .22f)), _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, const D2D1_COLOR_F & color, float opacity, float width) noexcept
    {
        SetBrush(color, opacity, 1.15f);

        renderTarget->DrawLine(from, to, _Brush.Get(), std::max(.6f, width), _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetBrush(const D2D1_COLOR_F & color, float opacity, float gain) noexcept
    {
        const float BrightnessFactor = [&]() noexcept
        {
            switch (_Brightness)
            {
                case Brightness::Low:
                    return 1.05f;

                default:
                case Brightness::Medium:
                    return 1.45f;

                case Brightness::High:
                    return 1.80f;
            }
        }();

        const float Boost = BrightnessFactor * gain;

        _Brush->SetColor(D2D1::ColorF(Clamp(color.r * Boost), Clamp(color.g * Boost), Clamp(color.b * Boost), 1.f));
        _Brush->SetOpacity(Clamp(opacity * color.a));
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if (_RenderTarget == renderTarget && _BackBufferRenderTarget && _BackBuffer && _Brush && _StrokeStyle)
        {
            const auto size = _BackBufferRenderTarget->GetPixelSize();

            if (size.width == (UINT32) _Width && size.height == (UINT32) _Height)
                return S_OK;
        }

        DeleteDeviceDependentResources();

        hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(Black, _Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const auto Style = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_SOLID, 0.f);

        hr = _D2DFactory->CreateStrokeStyle(Style, nullptr, 0, _StrokeStyle.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        {
            const auto DesiredSize = D2D1::SizeF(_Width, _Height);
            const auto PixelSize = D2D1::SizeU((UINT32) _Width, (UINT32) _Height);

            hr = renderTarget->CreateCompatibleRenderTarget(&DesiredSize, &PixelSize, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, _BackBufferRenderTarget.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;

            _BackBufferRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            _BackBufferRenderTarget->BeginDraw();

            _BackBufferRenderTarget->Clear(D2D1::ColorF(0.f, 0.f, .008f, 1.f));

            hr = _BackBufferRenderTarget->EndDraw();

            if (FAILED(hr))
                return hr;

            hr = _BackBufferRenderTarget->GetBitmap(_BackBuffer.ReleaseAndGetAddressOf());

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

        _BackBuffer.Reset();
        _BackBufferRenderTarget.Reset();

        _StrokeStyle.Reset();
        _Brush.Reset();

        __super::DeleteDeviceDependentResources();
    }
}

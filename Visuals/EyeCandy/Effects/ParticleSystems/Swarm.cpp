
/** $VER: Swarm.cpp (2026.10.23) */

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
        return std::format(L"Swarm, {} quality, {} speed", Qualities[(size_t)_Quality], Speeds[(size_t)_Speed]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        const size_t Count = (_Quality == Quality::Low) ? 420u : (_Quality == Quality::High) ? 1500u : 850u;

        _Agents.resize(Count);
        _Snapshot.resize(Count);

        for (size_t i = 0; i < Count; ++i)
            RespawnAgent(_Agents[i], i);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept
    {
        HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = Clamp(deltaTime);

        AnalyzeAudio(chunk);
        Update(deltaTime);

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

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk_impl & chunk)
    {
        _Peak          = 0.;
        _RMS           = 0.;

        _SpectralFlux  = 0.;
        _StereoBalance = 0.;

        _BandEnergy.fill(0.);

        _HasAudio = false;

        if (chunk.is_empty())
            return;

        const audio_sample * Frames = chunk.get_data();
        const unsigned ChannelCount = chunk.get_channel_count();
        const unsigned SampleRate = chunk.get_sample_rate();

        const size_t FrameCount = std::min(chunk.get_sample_count(), (size_t) 2048);

        if (!Frames || !ChannelCount || FrameCount < 32 || !SampleRate)
            return;

        std::vector<double> WindowedSamples(FrameCount);

        double SquareSum = 0.;
        double LeftPower = 0.;
        double RightPower = 0.;

        for (size_t i = 0; i < FrameCount; ++i)
        {
            const audio_sample * Frame = Frames + (i * ChannelCount);

            double Mono = 0.;

            for (unsigned c = 0; c < ChannelCount; ++c)
                Mono += Frame[c];

            Mono = ClampAudio(Mono / ChannelCount);

            WindowedSamples[i] = Mono * (.5 - .5 * std::cos(TwoPi * (double) i / ((double) FrameCount - 1.)));

            SquareSum += Mono * Mono;

            _Peak = std::max(_Peak, std::abs(Mono));

            LeftPower += Frame[0] * Frame[0];

            const double Right = Frame[ChannelCount > 1 ? 1 : 0];

            RightPower += Right * Right;
        }

        _RMS = std::sqrt(SquareSum / (double) FrameCount);

        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);

        if (!_HasAudio)
            return;

        const double StereoPower = LeftPower + RightPower;

        _StereoBalance = StereoPower > 1.e-12 ? std::clamp((RightPower - LeftPower) / StereoPower, -1., 1.) : 0.;

        const double NyquistFrequency = SampleRate * .5;

        double Maximum = 0.;

        for (size_t i = 0; i < BandCount; ++i)
        {
            const double Amount = (double) i / (double) (BandCount - 1.);
            const double Center = 40. * std::pow(400., Amount);

            double Power = 0.;

            for (double Ratio : { .890898718, 1., 1.122462048 })
            {
                const double Frequency = std::clamp(Center * Ratio, 20., NyquistFrequency * .94);
                const double Coefficient = 2. * std::cos(TwoPi * Frequency / SampleRate);

                double q1 = 0.;
                double q2 = 0.;

                for (double sample : WindowedSamples)
                {
                    const double q0 = sample + Coefficient * q1 - q2;

                    q2 = q1;
                    q1 = q0;
                }

                Power += std::max(0., q2 * q2 + q1 * q1 - Coefficient * q1 * q2);
            }

            const double Magnitude = std::sqrt(Power / 3.) / (double) FrameCount;

            _BandEnergy[i] = std::clamp(std::log1p(Magnitude * 80.) / std::log1p(80.) * Lerp(.9, 1.5, Amount), 0., 1.);

            Maximum = std::max(Maximum, _BandEnergy[i]);
        }

        if (Maximum > 1.e-5)
            for (double & e : _BandEnergy) e = std::clamp(e / std::max(.3, Maximum), 0., 1.);

        for (size_t i = 0; i < BandCount; ++i)
        {
            _SpectralFlux += std::max(0., _BandEnergy[i] - _PreviousBandEnergy[i]);
            _PreviousBandEnergy[i] = _BandEnergy[i];
        }

        _SpectralFlux = Clamp(_SpectralFlux / BandCount * 5.8);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        if (_Agents.empty())
            Reset();

        const double SpeedFactor = (_Speed == Speed::Slow) ? .55 : (_Speed == Speed::Fast) ? 1.75 : 1.;

        _AnimationTime += deltaTime * SpeedFactor;

        _SmoothedPeak          = ExponentialSmoothing(_SmoothedPeak,          _Peak,          deltaTime, _Peak         > _SmoothedPeak ? 24. : 5.);
        _SmoothedRMS           = ExponentialSmoothing(_SmoothedRMS,           _RMS,           deltaTime, _RMS          > _SmoothedRMS  ? 20. : 4.);
        _SmoothedFlux          = ExponentialSmoothing(_SmoothedFlux,          _SpectralFlux,  deltaTime, _SpectralFlux > _SmoothedFlux ? 36. : 6.);
        _SmoothedStereoBalance = ExponentialSmoothing(_SmoothedStereoBalance, _StereoBalance, deltaTime, 8.);

        _PreviousBeatPulse = _BeatPulse;

        _BeatPulse      = std::max(_BeatPulse * std::exp(-deltaTime * 7.2), _SpectralFlux);
        _BeatCooldown   = std::max(0., _BeatCooldown - deltaTime);

        const double BeatEdge = std::max(0., _BeatPulse - _PreviousBeatPulse);

        if (BeatEdge > .10 && _BeatCooldown <= 0.)
        {
            _PreviousPaletteIndex = _PaletteIndex;

            // Deliberately jump across the wheel instead of stepping to a neighboring palette.
            _PaletteIndex   = (_PaletteIndex + 2 + (size_t)(_BassEnergy * 3.5)) % 6;
            _PaletteBlend   = 0.;
            _BeatCooldown   = .12;
            _BeatColorPhase = Fraction(_BeatColorPhase + .16 + BeatEdge * .30);
        }

        _PaletteBlend   = std::min(1., _PaletteBlend + deltaTime * (7.5 + _BeatPulse * 10.));

        _BeatColorPhase = Fraction(_BeatColorPhase + deltaTime * (.018 + _SmoothedRMS * .045));
        _GradientPhase  = Fraction(_GradientPhase  + deltaTime * (.035 + _BeatPulse   * .12));

        for (size_t i = 0; i < BandCount; ++i)
            _SmoothedBandEnergy[i] = ExponentialSmoothing(_SmoothedBandEnergy[i], _HasAudio ? _BandEnergy[i] : 0., deltaTime, _BandEnergy[i] > _SmoothedBandEnergy[i] ? 22. : 4.5);

        _BassEnergy   = (_SmoothedBandEnergy[0] + _SmoothedBandEnergy[1] + _SmoothedBandEnergy[ 2] + _SmoothedBandEnergy[ 3]) * .25;
        _MidEnergy    = (_SmoothedBandEnergy[4] + _SmoothedBandEnergy[5] + _SmoothedBandEnergy[ 6] + _SmoothedBandEnergy[ 7]) * .25;
        _TrebleEnergy = (_SmoothedBandEnergy[8] + _SmoothedBandEnergy[9] + _SmoothedBandEnergy[10] + _SmoothedBandEnergy[11]) * .25;

        _Snapshot = _Agents;

        for (size_t i = 0; i < _Agents.size(); ++i)
            UpdateAgent(i, deltaTime * SpeedFactor);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateAgent(size_t index, double deltaTime) noexcept
    {
        agent_t & Agent = _Agents[index];

        const agent_t & s = _Snapshot[index];

        Agent.Previous = s.Position;
        Agent.Position = s.Position;
        Agent.Velocity = s.Velocity;

        const double Dimension  = std::max(1.f, std::min(_Width, _Height));
        const double Perception = Dimension * (.055 + _MidEnergy * .035);

        D2D1_POINT_2F Center {}, Alignment {}, Separation {};

        size_t Neighbors = 0;

        uint32_t State = Hash((uint32_t) index ^ s.Seed ^ 0xA511E9B3u);

        for (size_t n = 0; n < std::min(NeighborSamples, _Snapshot.size() - 1); ++n)
        {
            State = Hash(State + (uint32_t)n * 0x9E3779B9u);

            const size_t j = (index + 1 + State % (_Snapshot.size() - 1)) % _Snapshot.size();

            const auto & Snapshot = _Snapshot[j];

            const double dx = Snapshot.Position.x - s.Position.x;
            const double dy = Snapshot.Position.y - s.Position.y;

            const double Distance = Length(dx, dy);

            if (Distance < 1.e-3 || Distance > Perception)
                continue;

            Center.x += Snapshot.Position.x;
            Center.y += Snapshot.Position.y;

            Alignment.x += Snapshot.Velocity.x;
            Alignment.y += Snapshot.Velocity.y;

            if (Distance < Perception * .38)
            {
                const double k = 1. - Distance / (Perception * .38);

                Separation.x -= (float)(dx / Distance * k);
                Separation.y -= (float)(dy / Distance * k);
            }

            ++Neighbors;
        }

        double ax = 0., ay = 0.;

        if (Neighbors != 0)
        {
            const double Inv = 1. / (double) Neighbors;

            ax += (Center.x * Inv - s.Position.x) * (.42 + _MidEnergy * .72);
            ay += (Center.y * Inv - s.Position.y) * (.42 + _MidEnergy * .72);

            ax += (Alignment.x * Inv - s.Velocity.x) * (.75 + _TrebleEnergy * 1.25);
            ay += (Alignment.y * Inv - s.Velocity.y) * (.75 + _TrebleEnergy * 1.25);

            ax += Separation.x * Dimension * (1.8 + _BassEnergy * 3.2);
            ay += Separation.y * Dimension * (1.8 + _BassEnergy * 3.2);
        }

        const double Band     = (double) s.Band / (double) (BandCount - 1.);
        const double Energy   = _SmoothedBandEnergy[s.Band];
        const double BalanceX = _CenterX + _SmoothedStereoBalance * _Width * .29;
        const double LaneY    = _CenterY + std::sin(s.Phase + _AnimationTime * (.25 + Band)) * Dimension * (.08 + Band * .18);

        ax += (BalanceX - s.Position.x) * (.32 + _BassEnergy * .55);
        ay += (LaneY - s.Position.y) * (.20 + _MidEnergy * .42);

        const double Heading      = std::atan2(s.Velocity.y, s.Velocity.x);
        const double ImpulseAngle = Heading + ((Hash(s.Seed) & 1u) ? 1. : -1.) * (1.1 + Band * .9);
        const double Impulse      = Dimension * _BeatPulse * (1.2 + Energy * 2.4);

        ax += std::cos(ImpulseAngle) * Impulse;
        ay += std::sin(ImpulseAngle) * Impulse;

        const double Noise = s.Phase + _AnimationTime * (1.2 + Band * 2.8);

        ax += std::cos(Noise +        s.Position.y * .008) * Dimension * (.035 + _TrebleEnergy * .08);
        ay += std::sin(Noise * 1.17 + s.Position.x * .007) * Dimension * (.035 + _TrebleEnergy * .08);

        Agent.Velocity.x += (float) (ax * deltaTime);
        Agent.Velocity.y += (float) (ay * deltaTime);

        const double Drag = std::exp(-deltaTime * (1.05 - _SmoothedRMS * .25));

        Agent.Velocity.x *= (float) Drag;
        Agent.Velocity.y *= (float) Drag;

        const double MaxSpeed     = Dimension * (.34 + _SmoothedRMS * .42 + _BeatPulse * .28);
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

        // Wrapping teleports the agent to the opposite edge. Starting the trail
        // there prevents a spurious full-height or full-width line.
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

        const double beat = std::clamp(_BeatPulse, 0., 1.);
        const double drift = _GradientPhase * TwoPi;
        const double paletteHue = GetPaletteHue(.12, _PaletteBlend);

        // Animated vertical color field, blended lightly so persistent trails survive.
        for (int i = 0; i < Bands; ++i)
        {
            const double t = (double) i / (Bands - 1);

            const double centerGlow = 1. - std::abs(t * 2. - 1.);
            const double waveA      = .5 + .5 * std::sin(TwoPi * t * 1.4 + drift);
            const double waveB      = .5 + .5 * std::cos(TwoPi * t * 2.2 - drift * .63);
            const double hue        = Fraction(paletteHue + t * .12 + waveB * .025);
            const double value      = .008 + centerGlow * .024 + waveA * .018 + beat * centerGlow * .045;

            const uint32_t Color = HSVToBGRA((float) hue, .92f, (float) std::clamp(value, 0., .085));

            SetBrush(Color, (float)std::clamp(deltaTime * 5.2, .025, .16));

            const float y0 = (float)((float) i * _Height / Bands);

            renderTarget->FillRectangle(D2D1::RectF(0.f, y0, _Width, y0 + _Height / Bands + 1.f), _Brush.Get());
        }

        // Moving radial blooms make the gradient breathe, pan, and flash with each beat.
        const float Span = std::min(_Width, _Height);

        for (int Lobe = 0; Lobe < 3; ++Lobe)
        {
            const double Phase = drift + Lobe * TwoPi / 3.;

            const D2D1_POINT_2F center
            {
                (float)(_CenterX + std::sin(Phase * (1. + Lobe * .13) + _SmoothedStereoBalance * 1.8) * _Width * (.24 + beat * .15)),
                (float)(_CenterY + std::cos(Phase * (.73 + Lobe * .09)) * _Height * .24)
            };

            const float Radius = Span * (float)(.24 + Lobe * .065 + beat * .28);

            const uint32_t Color = HSVToBGRA((float)Fraction(paletteHue + Lobe * .115 + _BeatColorPhase * .18), .98f, 1.f);

            SetBrush(Color, (float)(.010 + beat * .018));

            renderTarget->FillEllipse(D2D1::Ellipse(center, Radius, Radius), _Brush.Get());

            SetBrush(Color, (float)(.012 + beat * .026));

            renderTarget->FillEllipse(D2D1::Ellipse(center, Radius * .48f, Radius * .48f), _Brush.Get());
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Persistence = .88 + _SmoothedRMS * .055 + _BeatPulse * .02;
        const double Decay = std::pow(std::clamp(Persistence, .78, .965), deltaTime * 60.);

        _Brush->SetColor(D2D1::ColorF(0.f, 0.f, .008f, (float)(1. - Decay)));
        _Brush->SetOpacity(1.f); renderTarget->FillRectangle(_Rectangle, _Brush.Get());
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
            const double Energy    = _SmoothedBandEnergy[Agent.Band];
            const double Intensity = Agent.Brightness * (.58 + Energy * 1.05 + _SmoothedRMS * .78 + _BeatPulse * .35) * std::clamp(Speed / (Side * .20), .58, 1.45);

            const uint32_t Color = Colorize(Agent.Band, Energy, Fraction(Agent.Phase / TwoPi + _AnimationTime * .025));
            const double trail = std::clamp(.45 + _SmoothedRMS * 1.05 + _BeatPulse * .8, .45, 1.9);

            const D2D1_POINT_2F Tail
            {
                (float) (Agent.Position.x - (Agent.Position.x - Agent.Previous.x) * trail),
                (float) (Agent.Position.y - (Agent.Position.y - Agent.Previous.y) * trail)
            };

            DrawNeonLine (renderTarget, Tail, Agent.Position, Color, (float) Intensity, (float)(Agent.Size * (1.25 + Energy * 1.9)));
            DrawNeonPoint(renderTarget,       Agent.Position, Color, (float)(Intensity * (.45 + Energy * .3)), (float)(Agent.Size * (1.7 + Energy * 2.6)));
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

            DrawNeonLine(renderTarget, q, p, HSVToBGRA((float)Fraction(.48 + Angle * .50), 1.f, 1.f), .32f, 1.35f);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    uint32_t effect_t::Colorize(size_t band, double energy, double phase) const noexcept
    {
        const double Amount   = (double) band / (double) (BandCount - 1.);
        const double Position = Fraction(Amount * .92 + phase * .08 + _SmoothedStereoBalance * .035);

        const float Hue = (float) GetPaletteHue(Position, _PaletteBlend);

        return HSVToBGRA(Hue, 1.f, (float)std::clamp(.96 + energy * .08 + _BeatPulse * .16, 0., 1.));
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

        if (Delta > .5) Delta -= 1.;
        if (Delta < -.5) Delta += 1.;

        const double Eased = blend * blend * (3. - 2. * blend);

        return Fraction(From + Delta * Eased + _BeatColorPhase);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawNeonLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float width) noexcept
    {
        const float q = _Quality == Quality::High ? 1.35f : (_Quality == Quality::Low ? .8f : 1.f);

        DrawLine(renderTarget, from, to, color, intensity * .055f, width * 9.f * q);
        DrawLine(renderTarget, from, to, color, intensity * .12f, width * 5.5f * q);
        DrawLine(renderTarget, from, to, color, intensity * .30f, width * 2.8f * q);
        DrawLine(renderTarget, from, to, color, intensity * .78f, width * 1.35f);

        SetBrush(0xFFFFFFFFu, std::clamp(intensity * .72f, 0.f, 1.f), 1.f);

        renderTarget->DrawLine(from, to, _Brush.Get(), std::max(.65f, width * .34f), _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawNeonPoint(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F pt, uint32_t color, float intensity, float radius) noexcept
    {
        if (radius <= 0.f || intensity <= .001f)
            return;

        SetBrush(color, intensity * .045f, 1.35f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius * 4.2f,  radius * 4.20f), _Brush.Get());
        SetBrush(color, intensity * .100f, 1.25f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius * 2.7f,  radius * 2.70f), _Brush.Get());
        SetBrush(color, intensity * .270f, 1.15f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius * 1.55f, radius * 1.55f), _Brush.Get());
        SetBrush(color, intensity * .880f, 1.05f); renderTarget->FillEllipse(D2D1::Ellipse(pt, radius *  .62f, radius *  .62f), _Brush.Get());

        SetBrush(0xFFFFFFFFu, intensity, 1.f); renderTarget->FillEllipse(D2D1::Ellipse(pt, std::max(.8f, radius * .22f), std::max(.8f, radius * .22f)), _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float opacity, float width) noexcept
    {
        SetBrush(color, opacity, 1.15f);
        renderTarget->DrawLine(from, to, _Brush.Get(), std::max(.6f, width), _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetBrush(uint32_t color, float opacity, float gain) noexcept
    {
        const float Mode = (_Brightness == Brightness::Low)  ? 1.05f : (_Brightness == Brightness::High) ? 1.80f : 1.45f;

        const float Boost = Mode * gain;

        // HSVToBGRA returns a Windows-style packed BGRA value whose numeric representation is 0xAARRGGBB.
        const float B = (float) ((color >>  0) & 0xFFu) / 255.f;
        const float G = (float) ((color >>  8) & 0xFFu) / 255.f;
        const float R = (float) ((color >> 16) & 0xFFu) / 255.f;
        const float A = (float) ((color >> 24) & 0xFFu) / 255.f;

        _Brush->SetColor(D2D1::ColorF(Clamp(R * Boost), Clamp(G * Boost), Clamp(B * Boost), 1.f));

        _Brush->SetOpacity(Clamp(opacity * A));
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

            if (size.width == (UINT32)_Width && size.height == (UINT32)_Height)
                return S_OK;
        }

        DeleteDeviceDependentResources();

        hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 1.f), _Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const auto Style = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_SOLID, 0.f);

        hr = _D2DFactory->CreateStrokeStyle(Style, nullptr, 0, _StrokeStyle.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const auto DesiredSize = D2D1::SizeF(_Width, _Height);
        const auto PixelSize = D2D1::SizeU((UINT32)_Width, (UINT32)_Height);

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

        _RenderTarget = renderTarget;

        Reset();

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


/** $VER: ParticleVortex.cpp (2026.10.20) **/

#include <pch.h>

#include "ParticleVortex.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace ParticleVortex
{
    effect_t::effect_t()
    {
        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const float Amount = (float) i / (float) (FrequencyBandCount - 1);

            _Centers[i].Frequency = 40. * std::pow(16'500. / 40., Amount); // 40 Hz to 16.5 kHz
            _Centers[i].Phase     = Amount * TwoPi;
        }

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
        return std::format(L"Particle Vortex, {} quality, {} speed, {} brightness", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _Particles.clear();

        const size_t ParticleCount = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low:    return  500u;
                default:
                case EyeCandy::Quality::Medium: return  950u;
                case EyeCandy::Quality::High:   return 1800u;
            }
        }();

        _Particles.resize(ParticleCount);

        for (size_t i = 0; i < _Particles.size(); ++i)
        {
            RespawnParticle(_Particles[i], i);

            uint32_t State = Hash((uint32_t)i ^ 0x9E3779B9u);

            _Particles[i].Age = Random(State) * _Particles[i].Lifetime;
        }
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

        {
            _BackBufferRenderTarget->BeginDraw();

            auto RenderTarget = _BackBufferRenderTarget.Get();

            Fade(RenderTarget, deltaTime);

            if (_HasAudio)
                RenderVortex(RenderTarget);
            else
                RenderIdleVortex(RenderTarget);

            RenderTarget = nullptr;

            if (FAILED(_BackBufferRenderTarget->EndDraw()))
            {
                DeleteDeviceDependentResources();

                return;
            }
        }

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _Peak         = 0.;
        _RMS          = 0.;
        _SpectralFlux = 0.;

        _BandEnergy.fill(0.);

        _HasAudio = false;

        if (chunk.is_empty())
            return;

        const audio_sample * const Frames = chunk.get_data();

        const unsigned ChannelCount  = chunk.get_channel_count();
        const unsigned ChannelConfig = chunk.get_channel_config();
        const unsigned SampleRate    = chunk.get_sample_rate();

        // Analyze only the last 2048 frames of the chunk.
        const size_t FrameCount = std::min(chunk.get_sample_count(), (size_t) 2048);

        if (FrameCount < 32)
            return;

        std::vector<double> WindowedSamples(FrameCount, 0.);

        std::array<double, audio_chunk::defined_channel_count> ChannelWeights {};
        double TotalChannelWeight = 0.;

        {
            unsigned ConfiguredChannelCount = 0;
            unsigned ChannelIndex = 0;

            for (unsigned ChannelBit = 0; (ChannelBit < audio_chunk::defined_channel_count) && (ChannelIndex < ChannelCount); ++ChannelBit)
            {
                const unsigned Channel = 1u << ChannelBit;

                if ((ChannelConfig & Channel) == 0)
                    continue;

                double Weight = 1.;

                switch (Channel)
                {
                    case audio_chunk::channel_front_center:
                        Weight = 1.;
                        break;

                    case audio_chunk::channel_front_left:
                    case audio_chunk::channel_front_right:
                    case audio_chunk::channel_front_center_left:
                    case audio_chunk::channel_front_center_right:
                        Weight = 1.;
                        break;

                    case audio_chunk::channel_back_left:
                    case audio_chunk::channel_back_right:
                    case audio_chunk::channel_back_center:
                    case audio_chunk::channel_side_left:
                    case audio_chunk::channel_side_right:
                        Weight = 0.7071067811865476;
                        break;

                    case audio_chunk::channel_top_center:
                    case audio_chunk::channel_top_front_left:
                    case audio_chunk::channel_top_front_center:
                    case audio_chunk::channel_top_front_right:
                    case audio_chunk::channel_top_back_left:
                    case audio_chunk::channel_top_back_center:
                    case audio_chunk::channel_top_back_right:
                        Weight = 0.5;
                        break;

                    case audio_chunk::channel_lfe:
                        Weight = 0.35;
                        break;

                    default:
                        Weight = 0.7071067811865476;
                        break;
                }

                ChannelWeights[ChannelIndex++] = Weight;

                TotalChannelWeight += Weight;

                ++ConfiguredChannelCount;
            }

            // A malformed, incomplete or custom channel mask must not cause channels at the end of each interleaved frame to be ignored.
            while (ChannelIndex < ChannelCount)
            {
                ChannelWeights[ChannelIndex++] = 1.;

                TotalChannelWeight += 1.;
            }

            // If no usable channel mask was supplied, use an equal average.
            if ((ConfiguredChannelCount == 0) || (TotalChannelWeight <= 0.))
            {
                TotalChannelWeight = (double) ChannelCount;

                for (unsigned Channel = 0; Channel < ChannelCount; ++Channel)
                    ChannelWeights[Channel] = 1.;
            }
        }

        double SquaredSum = 0.;

        {
            const double InverseChannelWeight = 1. / TotalChannelWeight;

            const size_t FirstFrame = FrameCount - FrameCount;

            const audio_sample *       Frame    = Frames + FirstFrame * ChannelCount;
            const audio_sample * const FrameEnd = Frame + FrameCount * ChannelCount;

            double * WindowedSample = WindowedSamples.data();

            size_t FrameIndex = 0;

            while (Frame < FrameEnd)
            {
                const double * Weight = ChannelWeights.data();

                double Mono = 0.;

                {
                    const audio_sample * Source = Frame;
                    const audio_sample * const SourceEnd = Source + ChannelCount;

                    while (Source < SourceEnd)
                        Mono += (double) *Source++ * *Weight++;

                    Mono *= InverseChannelWeight;
                }

                const double Sample = ClampAudio(Mono);

                // Apply a Hann window.
                {
                    const double Window = 0.5 - (std::cos(TwoPi * (double) FrameIndex / (double) (FrameCount - 1)) * 0.5);

                    *WindowedSample++ = (audio_sample) ((double) Sample * Window);
                }

                SquaredSum += Sample * Sample;

                _Peak = std::max(_Peak, std::abs(Sample));

                Frame += ChannelCount;

                ++FrameIndex;
            }
        }

        _RMS = std::sqrt(SquaredSum / (double) FrameCount);

        _HasAudio = (_Peak > 1.0e-5) || (_RMS > 1.0e-6);

        if (!_HasAudio)
            return;

        const double Nyquist = (double) SampleRate * 0.5;

        double MaximumEnergy = 0.;

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const double BandAmount      = (FrequencyBandCount > 1) ? (double) i / (double) (FrequencyBandCount - 1) : 0.; 
            const double CenterFrequency = 40. * std::pow(16'500. / 40., BandAmount); // 40 Hz to 16.5 kHz

            const double Frequencies[3] =
            {
                CenterFrequency / 1.122462048,
                CenterFrequency,
                CenterFrequency * 1.122462048
            };

            double BandPower = 0.;

            for (const double j : Frequencies)
            {
                const double Frequency   = std::clamp(j, 20., Nyquist * 0.95);
                const double Omega       = TwoPi * Frequency / (double) SampleRate;
                const double Coefficient = 2. * std::cos(Omega);

                double Previous  = 0.;
                double Previous2 = 0.;

                const double * Sample = WindowedSamples.data();
                const double * const SampleEnd = Sample + FrameCount;

                while (Sample < SampleEnd)
                {
                    const double Current = (double) *Sample++ + Coefficient * Previous - Previous2;

                    Previous2 = Previous;
                    Previous  = Current;
                }

                BandPower += std::max(0., Previous2 * Previous2 + Previous  * Previous - Coefficient * Previous * Previous2);
            }

            const double Magnitude  = std::sqrt(BandPower / 3.) / (double) FrameCount;
            const double Compressed = std::log1p(Magnitude * 80.) / std::log1p(80.);

            _BandEnergy[i] = Clamp(Compressed * Lerp(0.85, 1.45, BandAmount));

            MaximumEnergy = std::max(MaximumEnergy, _BandEnergy[i]);
        }

        if (MaximumEnergy > 1.0e-5)
        {
            const double Normalization = 1. / std::max(0.35, MaximumEnergy);

            double *       Energy    = _BandEnergy.data();
            double * const EnergyEnd = Energy + FrequencyBandCount;

            while (Energy < EnergyEnd)
            {
                *Energy = Clamp(*Energy * Normalization);

                ++Energy;
            }
        }

        {
            double Flux = 0.;

            const double *       Energy    = _BandEnergy.data();
            const double * const EnergyEnd = Energy + FrequencyBandCount;

            double * PreviousEnergy = _PreviousBandEnergy.data();

            while (Energy < EnergyEnd)
            {
                Flux += std::max(0., *Energy - *PreviousEnergy);

                *PreviousEnergy++ = *Energy++;
            }

            _SpectralFlux = Clamp(Flux / (double) FrequencyBandCount * 4.); // Boost the spectral flux artificially by 4 to make it react more strongly.
        }
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

        const double AnimationDelta = deltaTime * SpeedFactor;

        _AnimationTime += AnimationDelta;

        _SmoothedPeak = ExponentialSmoothing(_SmoothedPeak, _Peak,         deltaTime, (_Peak         > _SmoothedPeak) ? 22. : 5.);
        _SmoothedRMS  = ExponentialSmoothing(_SmoothedRMS,  _RMS,          deltaTime, (_RMS          > _SmoothedRMS)  ? 16. : 4.);
        _SmoothedFlux = ExponentialSmoothing(_SmoothedFlux, _SpectralFlux, deltaTime, (_SpectralFlux > _SmoothedFlux) ? 28. : 5.);

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const double target = _HasAudio ? _BandEnergy[i] : 0.;

            _SmoothedBandEnergy[i] = ExponentialSmoothing(_SmoothedBandEnergy[i], target, deltaTime, target > _SmoothedBandEnergy[i] ? 18.0f : 4.5f);
        }

        _BassEnergy   = (_SmoothedBandEnergy[0] + _SmoothedBandEnergy[1] + _SmoothedBandEnergy[ 2] + _SmoothedBandEnergy[ 3]) * 0.25;
        _MidEnergy    = (_SmoothedBandEnergy[4] + _SmoothedBandEnergy[5] + _SmoothedBandEnergy[ 6] + _SmoothedBandEnergy[ 7]) * 0.25;
        _TrebleEnergy = (_SmoothedBandEnergy[8] + _SmoothedBandEnergy[9] + _SmoothedBandEnergy[10] + _SmoothedBandEnergy[11]) * 0.25;

        UpdateCenters(AnimationDelta);

        GenerateWork(AnimationDelta);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::UpdateCenters(double deltaTime)
    {
        const float Dimension = std::min(_Width, _Height);

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            center_t & Center = _Centers[i];

            Center.Previous = Center.Position;

            const double Amount             = (double) i / (double) (FrequencyBandCount - 1);
            const double LowFrequencyWeight = 1. - Amount;
            const double Energy             = _SmoothedBandEnergy[i];
            const double Direction          = ((i & 1u) == 0u) ? 1. : -1.;

            Center.Phase += deltaTime * Lerp(0.16, 0.72, Amount) * Direction * (0.75 + Energy * 1.5);

            const double RingRadius  = Dimension * Lerp(0.09, 0.34, Amount);
            const double Pulse       = 1. + Energy * Lerp(0.18, 0.42, Amount) + _SmoothedFlux * 0.18;
            const double Angle       = Center.Phase + (_AnimationTime * Lerp(0.08, 0.22, Amount)) + (Amount * TwoPi);
            const double EllipticalY = 0.55 + LowFrequencyWeight * 0.18;
            const double Wobble      = std::sin((Angle * 2.) + (Amount * 7.) + (_AnimationTime * 0.31)) * Dimension * 0.025 * (0.25 + Energy);

            Center.Position =
            {
                (float) (_CenterX + std::cos(Angle) * RingRadius * Pulse + Wobble),
                (float) (_CenterY + std::sin(Angle) * RingRadius * Pulse * EllipticalY)
            };
            Center.Energy   = Energy;
            Center.Peak     = std::max(Energy, ExponentialSmoothing(Center.Peak, Energy, deltaTime, Energy > Center.Peak ? 24. : 3.));
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::UpdateParticle(size_t index, double deltaTime) noexcept
    {
        particle_t & particle = _Particles[index];

        particle.Previous = particle.Position;
        particle.Age     += deltaTime;

        if (particle.Age >= particle.Lifetime)
        {
            RespawnParticle(particle, index);
            return;
        }

        const size_t Band = particle.Band % FrequencyBandCount;

        const center_t & Center = _Centers[Band];

        const double dx = Center.Position.x - particle.Position.x;
              double dy = Center.Position.y - particle.Position.y;

        double Distance = std::max(Length(dx, dy), 1.);

        const double RadialX          = dx / Distance;
        const double RadialY          = dy / Distance;
        const double TangentDirection = ((Band + (size_t)particle.Seed) & 1u) == 0u ? 1.0f : -1.0f;
        const double TangentX         = -RadialY * TangentDirection;
        const double TangentY         =  RadialX * TangentDirection;
        const double BandAmount       = (float) Band / (float) (FrequencyBandCount - 1);
        const double Energy           = (float) Center.Energy;

        const double Dimension        = std::min(_Width, _Height);
        const double PreferredRadius  = Dimension * Lerp(0.055, 0.018, BandAmount) * (float) (particle.RadiusBias * (1. + (Energy * 0.8)));

        double Attraction = Lerp(2.8, 6.2, BandAmount) * (0.24 + Energy * 1.85);

        Attraction += (float) _SmoothedFlux * Lerp(7.0, 3.2, BandAmount);

        const double RadialAcceleration  = (Distance - PreferredRadius) * Attraction;
        const double OrbitalAcceleration = Dimension * Lerp(0.78, 1.85, BandAmount) * (0.18f + Energy * 1.35f);

        const double NoisePhase          = (float) particle.OrbitOffset + (float) _AnimationTime * Lerp(1.1, 4.2, BandAmount);
        const double NoiseStrength       = Dimension * 0.072f * (0.18f + Energy);
        const double NoiseX              = std::cos(NoisePhase         + particle.Position.y * 0.011f) * NoiseStrength;
        const double NoiseY              = std::sin(NoisePhase * 1.17f + particle.Position.x * 0.009f) * NoiseStrength;

        particle.Velocity.x += (float) ((RadialX * RadialAcceleration + TangentX * OrbitalAcceleration + NoiseX) * deltaTime);
        particle.Velocity.y += (float) ((RadialY * RadialAcceleration + TangentY * OrbitalAcceleration + NoiseY) * deltaTime);

        const double Drag = std::exp(-deltaTime * Lerp(1.35, 2.55, BandAmount));

        particle.Velocity.x *= (float) Drag;
        particle.Velocity.y *= (float) Drag;

        const double MaximumSpeed = Dimension * Lerp(0.58, 1.08, BandAmount) * (1.0f + Energy * 0.6f);
        const double Speed        = Length(particle.Velocity.x, particle.Velocity.y);

        if (Speed > MaximumSpeed)
        {
            const double Scale = MaximumSpeed / Speed;

            particle.Velocity.x *= (float) Scale;
            particle.Velocity.y *= (float) Scale;
        }

        particle.Position.x += (float) (particle.Velocity.x * deltaTime);
        particle.Position.y += (float) (particle.Velocity.y * deltaTime);

        const double Margin = Dimension * 0.20;

        const bool IsInvisble = (particle.Position.x < -Margin) || (particle.Position.y < -Margin) || (particle.Position.x > (_Width + Margin)) || (particle.Position.y > (_Height + Margin));

        if (IsInvisble)
            RespawnParticle(particle, index);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RespawnParticle(particle_t & particle, size_t index) noexcept
    {
        uint32_t State = Hash((uint32_t)index * 0x9E3779B9u ^ particle.Seed ^ (uint32_t)(_AnimationTime * 1'000.0));

        particle.Seed = State;
        particle.Band = (index + (size_t) (Random(State) * (float) FrequencyBandCount)) % FrequencyBandCount;

        const center_t & Center = _Centers[particle.Band];

        const double Dimension = std::min(_Width, _Height);
        const double Radius    = Dimension * Lerp(0.08, 0.52, std::sqrt((double) Random(State)));
        const double CenterX   = (Center.Position.x != 0.f) ? Center.Position.x : _CenterX;
        const double CenterY   = (Center.Position.y != 0.f) ? Center.Position.y : _CenterY;
        const double Angle     = Random(State) * TwoPi;

        particle.Position =
        {
            (float) (CenterX + std::cos(Angle) * Radius),
            (float) (CenterY + std::sin(Angle) * Radius * 0.72)
        };
        particle.Previous = particle.Position;

        const double InitialSpeed = Dimension * Lerp(0.035, 0.16, (double) Random(State));
        const double Direction    = ((particle.Band + index) & 1u) == 0u ? 1. : -1.;

        particle.Velocity =
        {
            (float) (-std::sin(Angle) * InitialSpeed * Direction),
            (float) ( std::cos(Angle) * InitialSpeed * Direction)
        };
        particle.Age         = 0.;
        particle.Lifetime    = Lerp(2.8, 8.5, Random(State));
        particle.OrbitOffset = Random(State) * TwoPi;
        particle.RadiusBias  = Lerp(0.55, 1.75, Random(State));
        particle.Brightness  = Lerp(0.45, 1.00, Random(State));
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Persistence = 0.902f + _BassEnergy * 0.026f + _SmoothedFlux * 0.018f;
        const double Decay       = std::pow(std::clamp(Persistence, 0.70, 0.965), (deltaTime * 60.));

        _Brush->SetColor(D2D1::ColorF(0.f, 0.f, 0.f, (float) Clamp(1. - Decay)));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RenderVortex(ID2D1RenderTarget * renderTarget) noexcept
    {
        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            if (_Centers[i].Energy > 0.025f)
                RenderCenterGlow(renderTarget, _Centers[i], i);
        }

        for (const particle_t & particle : _Particles)
        {
            const double LifeAmount      = Clamp(particle.Age / std::max(particle.Lifetime, 0.001));

            const double FadeIn          =       SmoothStep(0.00f, 0.08f, (float) LifeAmount);
            const double FadeOut         = 1.f - SmoothStep(0.72f, 1.00f, (float) LifeAmount);

            const double Speed           = Length(particle.Velocity.x, particle.Velocity.y);
            const double SpeedBrightness = std::clamp(Speed / (std::min(_Width, _Height) * 0.35), 0.15, 1.0);

            const size_t Band            = particle.Band % FrequencyBandCount;
            const double Energy          = _Centers[Band].Energy;
            const double ParticlePhase   = Fraction(particle.OrbitOffset / TwoPi + LifeAmount + _AnimationTime * 0.025);

            const auto   Color           = Colorize(Band, Energy, ParticlePhase);
            const double Intensity       = particle.Brightness * FadeIn * FadeOut * (0.18 + (Energy * 0.92)) * (0.55 + (SpeedBrightness * 0.65));
            const double Width           = 0.75 + (Energy * 1.8) + (_SmoothedFlux * 1.25);

            DrawGlowLine(renderTarget, particle.Previous, particle.Position, Color, (float) Intensity, (float) Width);
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RenderCenterGlow(ID2D1RenderTarget * renderTarget, const center_t & center, size_t band) noexcept
    {
        const double Energy = Clamp(center.Energy);

        const int MaxRadius = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low: return 7;
                case EyeCandy::Quality::High: return 15;
                case EyeCandy::Quality::Medium:
                default: return 10;
            }
        }();

        const int Radius = std::max(2, (int) std::lround(2. + (Energy * (double) MaxRadius)));

        const auto Color = Colorize(band, Energy, (float) (_AnimationTime * 0.03));

        DrawGlowPoint(renderTarget, center.Position, Color, (float) (Energy * 0.15),               (float) Radius);
        DrawGlowPoint(renderTarget, center.Position, Color, (float) (Energy * 0.52), std::max(1.f, (float) Radius / 4.f));
    }

    /// <summary>
    /// Renders a vortex with idle particles.
    /// </summary>
    void effect_t::RenderIdleVortex(ID2D1RenderTarget * renderTarget) noexcept
    {
        if ((_Width < 2.f) || (_Height < 2.f))
            return;

        const double Dimension = std::min(_Width, _Height);
        const D2D1_POINT_2F Center = { _CenterX, _CenterY };

        constexpr size_t Count = 96;

        for (size_t i = 0; i < Count; ++i)
        {
            const double Amount = (double) i / (double)(Count - 1);
            const double Phase  = (Amount * TwoPi * 7.) + _AnimationTime * (0.28 + (Amount * 0.75));
            const double Radius = Dimension * (0.035 + Amount * 0.31);
            const double Spiral = 1. + 0.08 * std::sin(Phase * 0.43 - _AnimationTime * 0.6);

            const D2D1_POINT_2F Current  = { (float) (Center.x + std::cos(Phase)         * Radius * Spiral), (float) (Center.y + std::sin(Phase)         * Radius * Spiral * 0.68) };
            const D2D1_POINT_2F Previous = { (float) (Center.x + std::cos(Phase - 0.045) * Radius * Spiral), (float) (Center.y + std::sin(Phase - 0.045) * Radius * Spiral * 0.68) };

            DrawGlowLine(renderTarget, Previous, Current, HSVToColor((float) (0.53 + Amount * 0.22), 0.82f, 0.58f), 0.072f, 1.f);
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, const D2D1_COLOR_F & color, float intensity, float width) noexcept
    {
        const int QualityRadius = [&]() noexcept
        {
            switch (_Quality)
            {
            case EyeCandy::Quality::Low:
                return 1;

            case EyeCandy::Quality::High:
                return 3;

            case EyeCandy::Quality::Medium:
            default:
                return 2;
            }
        }();

        const int Radius = std::max(1, (int) std::lround(width * 1.25f) + QualityRadius);

        // A restrained wide pass provides softness without obscuring the brighter, narrow particle trail.
        DrawLine(renderTarget, from, to, color, intensity * 0.055f, Radius * 2);
        DrawLine(renderTarget, from, to, color, intensity * 0.140f, Radius);
        DrawLine(renderTarget, from, to, color, intensity * 0.340f, std::max(1, Radius / 2));
        DrawLine(renderTarget, from, to, color, intensity * 0.940f, 0);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, const D2D1_COLOR_F & color, float intensity, int radius) noexcept
    {
        intensity = std::clamp(intensity, 0.f, 1.35f);

        if (intensity <= 0.0001f)
            return;

        // Opacity itself is limited to one. Values above one are converted into an additional brightness boost in SetBrush.
        SetBrush(color, intensity);

        const float StrokeWidth = (radius > 0) ? (float) (radius * 2 + 1) : 1.f;

        renderTarget->DrawLine(from, to, _Brush.Get(), StrokeWidth, _StrokeStyle.Get());
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::DrawGlowPoint(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F position, const D2D1_COLOR_F & color, float intensity, float radius) noexcept
    {
        if (radius <= 0)
            return;

        intensity = std::clamp(intensity, 0.f, 1.35f);

        if (intensity <= 0.0001f)
            return;

        const float BlurRadius   =                 (float) radius * 1.45f;
        const float OuterRadius  =                 (float) radius * 1.10f;
        const float MiddleRadius = std::max(1.00f, (float) radius * 0.68f);
        const float InnerRadius  = std::max(0.75f, (float) radius * 0.28f);

        const D2D1_POINT_2F Center = position;

        SetBrush(color, intensity * 0.055f);

        renderTarget->FillEllipse(D2D1::Ellipse(Center, BlurRadius, BlurRadius), _Brush.Get());

        SetBrush(color, intensity * 0.130f);

        renderTarget->FillEllipse(D2D1::Ellipse(Center, OuterRadius, OuterRadius), _Brush.Get());

        SetBrush(color, intensity * 0.340f);

        renderTarget->FillEllipse(D2D1::Ellipse(Center, MiddleRadius, MiddleRadius), _Brush.Get());

        SetBrush(color, intensity * 0.950f);

        renderTarget->FillEllipse(D2D1::Ellipse(Center, InnerRadius, InnerRadius), _Brush.Get());
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::SetBrush(const D2D1_COLOR_F & color, float opacity) noexcept
    {
        const float BrightnessFactor = [&]() noexcept
        {
            switch (_Brightness)
            {
                case Brightness::Low:
                    return 0.78f;

                case Brightness::High:
                    return 1.28f;

                case Brightness::Medium:
                default:
                    return 1.0f;
            }
        }();

        _Brush->SetColor(D2D1::ColorF(Clamp(color.r * BrightnessFactor), Clamp(color.g * BrightnessFactor), Clamp(color.b * BrightnessFactor), color.a));
        _Brush->SetOpacity(Clamp(opacity));
    }

    /// <summary>
    ///
    /// </summary>
    D2D1_COLOR_F effect_t::Colorize(size_t band, double energy, double particlePhase) const noexcept
    {
        const double BandAmount = (double) band / (double) (FrequencyBandCount - 1);

        const float H = (float) Fraction(0.92 + (BandAmount * 0.62) + (particlePhase * 0.055) + (_AnimationTime * 0.006));
        const float S = (float) Clamp(0.76 + (energy * 0.19));
        const float V = (float) Clamp(0.54 + (energy * 0.44) + (_SmoothedFlux * 0.16));

        return HSVToColor(H, S, V);
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
            for (size_t index = 0; index < WorkerCount; ++index)
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
            double DeltaTime;

            {
                std::unique_lock lock(_WorkerMutex);

                _WorkerCondition.wait(lock, [this, &Generation]() noexcept { return _IsStopping || _Generation != Generation; });

                if (_IsStopping)
                    return;

                DeltaTime  = _ParticleDeltaTime;

                Generation = _Generation;
            }

            for (;;)
            {
                const size_t ParticleIndex = _NextParticle.fetch_add(1, std::memory_order_relaxed);

                if (ParticleIndex >= _Particles.size())
                    break;

                UpdateParticle(ParticleIndex, DeltaTime);
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
    void effect_t::GenerateWork(double deltaTime)
    {
        if (_Particles.empty())
            Reset();

        if (_Particles.empty() || _Workers.empty())
            return;

        {
            const std::lock_guard Lock(_WorkerMutex);

            _ParticleDeltaTime = deltaTime;

            _NextParticle.store(0, std::memory_order_relaxed);

            _PendingWorkers = _Workers.size();
            ++_Generation;
        }

        _WorkerCondition.notify_all();

        {
            std::unique_lock Lock(_WorkerMutex);

            _CompletionCondition.wait(Lock, [this]() noexcept { return _PendingWorkers == 0; });
        }
    }

    /// <summary>
    ///
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr =__super::CreateDeviceDependentResources(renderTarget);

        if ((_RenderTarget == renderTarget) && _BackBufferRenderTarget && _BackBuffer && _Brush && _StrokeStyle)
        {
            const D2D1_SIZE_U Size = _BackBufferRenderTarget->GetPixelSize();

            if ((Size.width == (UINT32) _Width) && (Size.height == (UINT32) _Height))
                return S_OK;
        }

        {
            hr = renderTarget->CreateSolidColorBrush(Black, _Brush.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f);

            hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _StrokeStyle.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            const D2D1_SIZE_F DesiredSize      = D2D1::SizeF(_Width, _Height);
            const D2D1_SIZE_U DesiredPixelSize = D2D1::SizeU((UINT32) _Width, (UINT32) _Height);

            hr = renderTarget->CreateCompatibleRenderTarget(&DesiredSize, &DesiredPixelSize, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, _BackBufferRenderTarget.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;

            {
                _BackBufferRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

                _BackBufferRenderTarget->BeginDraw();

                _BackBufferRenderTarget->Clear(Black);

                hr = _BackBufferRenderTarget->EndDraw();

                if (FAILED(hr))
                    return hr;

                hr = _BackBufferRenderTarget->GetBitmap(_BackBuffer.ReleaseAndGetAddressOf());

                if (FAILED(hr))
                    return hr;
            }
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

        _StrokeStyle.Reset();
        _Brush.Reset();

        _BackBuffer.Reset();
        _BackBufferRenderTarget.Reset();

        __super::DeleteDeviceDependentResources();
    }
}

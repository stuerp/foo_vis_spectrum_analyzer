
/** $VER: Fireflies.cpp (2026.10.18) **/

#include <pch.h>

#include "Fireflies.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Fireflies
{
    namespace
    {
        struct palette_stop_t
        {
            float Position = 0.f;
            hsv_t Color{};
        };

        constexpr std::array WarmPalette
        {
            palette_stop_t { 0.00f, { 0.025f, 0.82f, 0.68f } },
            palette_stop_t { 0.24f, { 0.065f, 0.80f, 0.82f } },
            palette_stop_t { 0.52f, { 0.120f, 0.76f, 0.96f } },
            palette_stop_t { 0.76f, { 0.180f, 0.72f, 0.86f } },
            palette_stop_t { 1.00f, { 0.255f, 0.68f, 0.72f } }
        };

        constexpr std::array CoolPalette
        {
            palette_stop_t { 0.00f, { 0.470f, 0.74f, 0.72f } },
            palette_stop_t { 0.25f, { 0.525f, 0.80f, 0.86f } },
            palette_stop_t { 0.52f, { 0.590f, 0.84f, 0.98f } },
            palette_stop_t { 0.77f, { 0.660f, 0.78f, 0.88f } },
            palette_stop_t { 1.00f, { 0.745f, 0.70f, 0.76f } }
        };

        template <size_t Size> hsv_t SamplePalette(const std::array<palette_stop_t, Size> & palette, float position) noexcept
        {
            position = Clamp(position);

            if (position <= palette.front().Position)
                return palette.front().Color;

            for (size_t i = 1; i < Size; ++i)
            {
                if (position > palette[i].Position)
                    continue;

                const palette_stop_t & From = palette[i - 1];
                const palette_stop_t & To   = palette[i];

                const float Range  = std::max(To.Position - From.Position, 0.0001f);
                const float Amount = std::clamp((position - From.Position) / Range, 0.0f, 1.0f);

                return
                {
                    InterpolateHue(From.Color.H, To.Color.H, Amount),

                    Lerp(From.Color.S, To.Color.S, Amount),
                    Lerp(From.Color.V, To.Color.V, Amount)
                };
            }

            return palette.back().Color;
        }
    }

    effect_t::effect_t()
    {
        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const double Amount = (double) i / (double) (FrequencyBandCount - 1);

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
    /// Returns a human-readable description of the active effect settings.
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        constexpr std::array<const wchar_t *, 2> PaletteNames = {{ L"Warm", L"Cool" }};

        return std::format(L"Fireflies, {} palette, {} quality, {} speed, {} brightness, {} density",
            PaletteNames[(size_t) _Palette], Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness], DensityNames[(size_t) _Density]);
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
                case EyeCandy::Quality::Low: return 180u;
                case EyeCandy::Quality::High: return 720u;
                case EyeCandy::Quality::Medium:
                default: return 360u;
            }
        }();

        const float DensityMultiplier = [&]() noexcept
        {
            switch (_Density)
            {
                case Density::Low:
                    return 0.55f;

                case Density::High:
                    return 1.75f;

                case Density::Medium:
                default:
                    return 1.0f;
            }
        }();

        _Particles.resize(std::max<size_t>(1, (size_t) std::lround((float) ParticleCount * DensityMultiplier)));

        for (size_t i = 0; i < _Particles.size(); ++i)
        {
            RespawnParticle(_Particles[i], i);

            uint32_t State = Hash((uint32_t) i ^ 0x9E3779B9u);

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

            ID2D1RenderTarget * RenderTarget = _BackBufferRenderTarget.Get();

            Fade(RenderTarget, deltaTime);

            if (_HasAudio)
                RenderFireflies(RenderTarget);
            else
                RenderIdleFireflies(RenderTarget);

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

        const size_t FrameCount     = chunk.get_sample_count();
        const unsigned ChannelCount = chunk.get_channel_count();
        const unsigned SampleRate   = chunk.get_sample_rate();

        // Analyze only the most recent 2048 sample frames.
        const size_t AnalysisCount = std::min(FrameCount, (size_t) 2048);

        if (AnalysisCount < 32)
            return;

        const size_t FirstFrame = FrameCount - AnalysisCount;

        std::vector<double> WindowedSamples(AnalysisCount, 0.);

        const audio_sample *       Frame    = Frames + FirstFrame * (size_t) ChannelCount;
        const audio_sample * const FrameEnd = Frame + AnalysisCount * (size_t) ChannelCount;

        double * WindowedSample = WindowedSamples.data();

        double SquaredSum = 0.;

        {
            size_t FrameIndex = 0;

            while (Frame < FrameEnd)
            {
                double Mono = 0.;

                {
                    const audio_sample * Source = Frame;
                    const audio_sample * const SourceEnd = Source + ChannelCount;

                    while (Source < SourceEnd)
                        Mono += (double) *Source++;

                    Mono /= (double) ChannelCount;
                }

                const double Sample = ClampAudio(Mono);

                // Apply a Hann window.
                {
                    const double Window = 0.5 - (std::cos(TwoPi * (double) FrameIndex / (double) (AnalysisCount - 1)) * 0.5);

                    *WindowedSample++ = Sample * Window;
                }

                SquaredSum += Sample * Sample;

                _Peak = std::max(_Peak, std::abs(Sample));

                Frame += ChannelCount;

                ++FrameIndex;
            }
        }

        _RMS = std::sqrt(SquaredSum / (double) AnalysisCount);

        _HasAudio = (_Peak > 1.0e-5) || (_RMS > 1.0e-6);

        if (!_HasAudio)
            return;

        const double Nyquist = (double) SampleRate * 0.5;

        double MaximumEnergy = 0.;

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const double BandAmount = (FrequencyBandCount > 1) ? (double) i / (double) (FrequencyBandCount - 1) : 0.;

            const double CenterFrequency = 40. * std::pow(16'500. / 40., BandAmount); // 40 Hz to 16.5 kHz

            const double Frequencies[] =
            {
                CenterFrequency / 1.122462048,
                CenterFrequency,
                CenterFrequency * 1.122462048
            };

            double BandPower = 0.;

            const double * Frequency = Frequencies;
            const double * const FrequencyEnd = Frequency + std::size(Frequencies);

            while (Frequency < FrequencyEnd)
            {
                const double ClampedFrequency = std::clamp(*Frequency++, 20., Nyquist * 0.95);

                const double Omega = TwoPi * ClampedFrequency / (double) SampleRate;

                const double Coefficient = 2. * std::cos(Omega);

                double Previous  = 0.;
                double Previous2 = 0.;

                const double *       Sample    = WindowedSamples.data();
                const double * const SampleEnd = Sample + AnalysisCount;

                while (Sample < SampleEnd)
                {
                    const double Current = (double)*Sample++ + Coefficient * Previous - Previous2;

                    Previous2 = Previous;
                    Previous = Current;
                }

                BandPower += std::max(0., Previous2 * Previous2 + Previous * Previous - Coefficient * Previous * Previous2);
            }

            const double Magnitude  = std::sqrt(BandPower / (double) _countof(Frequencies)) / (double) AnalysisCount;
            const double Compressed = std::log1p(Magnitude * 80.) / std::log1p(80.);

            _BandEnergy[i] = (float) Clamp(Compressed * Lerp(0.85, 1.45, BandAmount));

            MaximumEnergy = std::max(MaximumEnergy, _BandEnergy[i]);
        }

        if (MaximumEnergy > 1.0e-5f)
        {
            const auto Normalization = 1.f / std::max(0.35, MaximumEnergy);

            double * Energy = _BandEnergy.data();
            double * const EnergyEnd = Energy + FrequencyBandCount;

            while (Energy < EnergyEnd)
            {
                *Energy = Clamp(*Energy * Normalization);

                ++Energy;
            }
        }

        double Flux = 0.;

        const double * Energy = _BandEnergy.data();
        const double * const EnergyEnd = Energy + FrequencyBandCount;

        double * PreviousEnergy = _PreviousBandEnergy.data();

        while (Energy < EnergyEnd)
        {
            Flux += std::max(0., *Energy - *PreviousEnergy);

            *PreviousEnergy++ = *Energy++;
        }

        _SpectralFlux = Clamp(Flux / (double) FrequencyBandCount * 4.); // Boost the spectral flux artificially by 4 to make it react more strongly.
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

        _PaletteTransition = std::min(1., _PaletteTransition + deltaTime / PaletteTransitionDuration);

        _SmoothedPeak         = ExponentialSmoothing(_SmoothedPeak,         _Peak,         deltaTime, (_Peak         > _SmoothedPeak)         ? 34. : 7.0);
        _SmoothedRMS          = ExponentialSmoothing(_SmoothedRMS,          _RMS,          deltaTime, (_RMS          > _SmoothedRMS)          ? 25. : 5.5);
        _SmoothedSpectralFlux = ExponentialSmoothing(_SmoothedSpectralFlux, _SpectralFlux, deltaTime, (_SpectralFlux > _SmoothedSpectralFlux) ? 42. : 7.0);

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const double Target = _HasAudio ? _BandEnergy[i] : 0.;

            _SmoothedBandEnergy[i] = ExponentialSmoothing(_SmoothedBandEnergy[i], Target, deltaTime, (Target > _SmoothedBandEnergy[i]) ? 30. : 6.5);
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

            const double Amount = (float)i / (float)(FrequencyBandCount - 1);
            const double Energy = _SmoothedBandEnergy[i];

            const double PreviousEnergy = i > 0 ? _SmoothedBandEnergy[i - 1] : _SmoothedBandEnergy[FrequencyBandCount - 1];
            const double NextEnergy     = i + 1 < FrequencyBandCount ? _SmoothedBandEnergy[i + 1] : _SmoothedBandEnergy[0];

            const bool localMaximum = Energy >= PreviousEnergy && Energy >= NextEnergy && Energy > 0.060f;

            Center.IsActive = localMaximum;
            Center.Energy = ExponentialSmoothing(Center.Energy, localMaximum ? Energy : Energy * 0.16f, deltaTime, localMaximum ? 36. : 6.);

            Center.Peak = std::max(Energy, ExponentialSmoothing(Center.Peak, Energy, deltaTime, (Energy > Center.Peak) ? 44. : 3.5));

            Center.Phase += (float)deltaTime * Lerp(0.18, 0.52, Amount) * ((i & 1u) == 0u ? 1.0f : -1.0f) * (0.80f + Center.Energy * 1.70f);

            const double Angle = Amount * TwoPi + Center.Phase * 0.28f;

            const double Radius = Dimension * (0.09f + Center.Energy * 0.28f);
            const double Drift = std::sin(Center.Phase * 1.7f + Amount * 11.0f) * Dimension * 0.018f;

            Center.Position =
            {
                (float) (_CenterX + std::cos(Angle) * (Radius + Drift)),
                (float) (_CenterY + std::sin(Angle) * (Radius + Drift) * 0.78)
            };
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::UpdateParticle(size_t index, double deltaTime) noexcept
    {
        particle_t & Particle = _Particles[index];

        const double Dimension = std::min(_Width, _Height);

        Particle.Previous = Particle.Position;
        Particle.Age     += deltaTime;

        if (Particle.Age >= Particle.Lifetime)
        {
            RespawnParticle(Particle, index);
            return;
        }

        size_t strongestBand = Particle.Band % FrequencyBandCount;

        double strongestInfluence = -1.;

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            const center_t & Center = _Centers[i];

            const double dx              = Center.Position.x - Particle.Position.x;
            const double dy              = Center.Position.y - Particle.Position.y;
            const double DistanceSquared = (dx * dx) + (dy * dy) + 64.;
            const double Influence       = Center.Energy * Center.Energy / DistanceSquared;

            if (Influence > strongestInfluence)
            {
                strongestInfluence = Influence;
                strongestBand = i;
            }
        }

        Particle.Band = strongestBand;

        const center_t & Center = _Centers[strongestBand];

        const double dx = Center.Position.x - Particle.Position.x;
        const double dy = Center.Position.y - Particle.Position.y;

        const double distance = std::max(Length(dx, dy), 1.);

        const double radialX             = dx / distance;
        const double radialY             = dy / distance;
        const double tangentDirection    = ((strongestBand + (size_t) Particle.Seed) & 1u) == 0u ? 1.0f : -1.0f;
        const double tangentX            = -radialY * tangentDirection;
        const double tangentY            =  radialX * tangentDirection;
        const double energy              = Center.Energy;
        const double preferredRadius     = Dimension * Particle.RadiusBias * Lerp(0.018, 0.065, 1. - energy);
        const double radialAcceleration  = (distance - preferredRadius) * (2.6f + energy * 12.8f + _SmoothedSpectralFlux * 4.2f);
        const double orbitalAcceleration = Dimension * (0.15f + energy * 1.05f + _SmoothedSpectralFlux * 0.25f);

        const double noisePhase          = Particle.OrbitOffset + (float) _AnimationTime * (1.8f + 4.4f * energy + _SmoothedSpectralFlux * 1.8f);
        const double noiseStrength       = Dimension * (0.030f + 0.092f * energy + _SmoothedSpectralFlux * 0.022f);
        const double noiseX              = std::cos(noisePhase + Particle.Position.y * 0.017f) * noiseStrength;
        const double noiseY              = std::sin(noisePhase * 1.31f + Particle.Position.x * 0.013f) * noiseStrength;

        Particle.Velocity.x += (float) ((radialX * radialAcceleration + tangentX * orbitalAcceleration + noiseX) * deltaTime);
        Particle.Velocity.y += (float) ((radialY * radialAcceleration + tangentY * orbitalAcceleration + noiseY) * deltaTime);

        const double Drag = std::exp(-deltaTime * (1.80f - energy * 0.72f));

        Particle.Velocity.x *= (float) Drag;
        Particle.Velocity.y *= (float) Drag;

        const double MaximumSpeed = Dimension * (0.44f + energy * 1.00f + _SmoothedSpectralFlux * 0.32f);
        const double Speed = Length(Particle.Velocity.x, Particle.Velocity.y);

        if (Speed > MaximumSpeed)
        {
            const double Scale = MaximumSpeed / Speed;

            Particle.Velocity.x *= (float) Scale;
            Particle.Velocity.y *= (float) Scale;
        }

        Particle.Position.x += Particle.Velocity.x * (float) deltaTime;
        Particle.Position.y += Particle.Velocity.y * (float) deltaTime;

        const double Margin = Dimension * 0.12;

        const bool IsInvisible = Particle.Position.x < -Margin || Particle.Position.y < -Margin || Particle.Position.x > (_Width + Margin) || Particle.Position.y > (_Height + Margin);

        if (IsInvisible)
            RespawnParticle(Particle, index);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RespawnParticle(particle_t & particle, size_t index) noexcept
    {
        uint32_t State = Hash((uint32_t)index * 0x9E3779B9u ^ particle.Seed ^ (uint32_t)(_AnimationTime * 1'000.0));

        particle.Seed = State;

        size_t SelectedBand = index % FrequencyBandCount;
        double TotalEnergy = 0.f;

        for (double Energy : _SmoothedBandEnergy)
            TotalEnergy += 0.035f + (Energy * Energy);

        double Selection = Random(State) * TotalEnergy;

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            Selection -= 0.035f + (_SmoothedBandEnergy[i] * _SmoothedBandEnergy[i]);

            if (Selection <= 0.f)
            {
                SelectedBand = i;
                break;
            }
        }

        particle.Band = SelectedBand;

        const center_t & Center = _Centers[particle.Band];

        const double Dimension = std::min(_Width, _Height);
        const double CenterX   = (Center.Position.x != 0.f) ? Center.Position.x : _CenterX;
        const double CenterY   = (Center.Position.y != 0.f) ? Center.Position.y : _CenterY;
        const double Angle     = Random(State) * TwoPi;
        const double Radius    = Dimension * Lerp(0.025, 0.22, std::sqrt(Random(State)));

        particle.Position =
        {
            (float) (CenterX + std::cos(Angle) * Radius),
            (float) (CenterY + std::sin(Angle) * Radius)
        };

        particle.Previous = particle.Position;

        const double InitialSpeed = Dimension * Lerp(0.018, 0.085, Random(State));

        particle.Velocity =
        {
            (float) (-std::sin(Angle) * InitialSpeed + RandomSigned(State) * InitialSpeed * 0.35f),
            (float) ( std::cos(Angle) * InitialSpeed + RandomSigned(State) * InitialSpeed * 0.35f)
        };

        particle.Age          = 0.;
        particle.Lifetime     = Lerp(2.2, 7.5, Random(State));
        particle.OrbitOffset  = Random(State) * TwoPi;
        particle.RadiusBias   = Lerp(0.45, 1.45, Random(State));
        particle.Brightness   = Lerp(0.74, 1.18, Random(State));
        particle.TwinklePhase = Random(State) * TwoPi;
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Persistence = 0.84 + (_SmoothedRMS * 0.035) + (_SmoothedSpectralFlux * 0.020);
        const double Decay       = std::pow(std::clamp(Persistence, 0.70, 0.965), (float) (deltaTime * 60.0));

        _Brush->SetColor(D2D1::ColorF(0.f, 0.f, 0.f, (float) Clamp(1. - Decay)));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RenderFireflies(ID2D1RenderTarget * renderTarget) noexcept
    {
        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            if (_Centers[i].Energy > 0.06f)
                RenderCenterGlow(renderTarget, _Centers[i], i);
        }

        for (const particle_t & particle : _Particles)
        {
            const size_t Band = particle.Band % FrequencyBandCount;

            const double Energy        = _Centers[Band].Energy;
            const double LifeAmount    = Clamp(particle.Age / std::max(particle.Lifetime, 0.001));
            const double FadeIn        =      SmoothStep(0.00f, 0.10f, (float) LifeAmount);
            const double FadeOut       = 1. - SmoothStep(0.72f, 1.00f, (float) LifeAmount);
            const double Twinkle       = 0.70 + 0.30 * std::sin(particle.TwinklePhase + _AnimationTime * (4.2 + Energy * 7.8));

            const double AudioResponse = 0.52 + (Energy * 1.42) + (_SmoothedRMS * 0.44) + (_SmoothedPeak * 0.18) + (_SmoothedSpectralFlux * 0.78);
            const double Intensity     = particle.Brightness * FadeIn * FadeOut * Twinkle * AudioResponse;
            const double ParticlePhase = Fraction((particle.OrbitOffset / TwoPi) + LifeAmount + (_AnimationTime * 0.018));

            const auto Color = Colorize(Band, (float) Energy, (float) ParticlePhase);

            const int Radius = std::max(1, (int) std::lround(1.35 + (Energy * 3.5) + (_SmoothedSpectralFlux * 1.75) + (particle.Brightness * 1.75)));

            DrawGlowPoint(renderTarget, particle.Position, Color, (float) Intensity, (float) Radius);

            const double Speed = Length(particle.Velocity.x, particle.Velocity.y);

            if (Speed > (std::min(_Width, _Height) * 0.08f))
                DrawGlowLine(renderTarget, particle.Previous, particle.Position, Color, (float) (Intensity * 0.32), 0.85f);
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RenderCenterGlow(ID2D1RenderTarget * renderTarget, const center_t & center, size_t band) noexcept
    {
        int Radius = [&]() noexcept
        {
            switch (_Quality)
            {
                case EyeCandy::Quality::Low:
                    return 10;

                case EyeCandy::Quality::High:
                    return 24;

                case EyeCandy::Quality::Medium:
                default:
                    return 16;
            }
        }();

        const double Energy = Clamp(center.Energy);

        Radius = std::max(3, (int) std::lround(3. + (Energy * (double) Radius)));

        const auto Color = Colorize(band, (float) std::min(1., Energy * 1.20), (float) (_AnimationTime * 0.03));

        DrawGlowPoint(renderTarget, center.Position, Color, (float) (Energy * 0.30 + _SmoothedSpectralFlux * 0.08),               (float) Radius);
        DrawGlowPoint(renderTarget, center.Position, Color, (float) (Energy * 0.90 + _SmoothedSpectralFlux * 0.14), std::max(2.f, (float) Radius / 3.f));
        DrawGlowPoint(renderTarget, center.Position, White, (float) (Energy * 0.74 + _SmoothedSpectralFlux * 0.18), std::max(1.f, (float) Radius / 7.f));
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::RenderIdleFireflies(ID2D1RenderTarget * renderTarget) noexcept
    {
        if ((_Width < 2.f) || (_Height < 2.f))
            return;

        constexpr size_t IdleParticles = 42;

        for (size_t i = 0; i < IdleParticles; ++i)
        {
            const double Amount = (double) i / (double) (IdleParticles - 1);
            const double Phase  = (Amount * TwoPi * 5.) + (_AnimationTime * (0.22 + Amount * 0.48));

            const D2D1_POINT_2F Position =
            {
                (float) (_Width  * (0.08 + Amount * 0.84)         + std::sin(Phase * 1.37) * _Width * 0.025),
                (float) (_Height * (0.50 + std::sin(Phase) * 0.28 + std::cos(Phase * 0.43)          * 0.080))
            };

            const auto Color = HSVToColor((float) (0.11 + Amount * 0.08), 0.72f, 0.78f);

            const double Twinkle = 0.55 + (0.45 * std::sin(Phase * 2.3));

            const float Opacity = (float) (0.10 + (Twinkle * 0.13));

            DrawGlowPoint(renderTarget, Position, Color, Opacity, 2);
        }
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, const D2D1_COLOR_F & color, float intensity, float width) noexcept
    {
        int Radius = [&]() noexcept
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

        Radius = std::max(1, (int) std::lround(width * 1.25f) + Radius);

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
        intensity = std::clamp(intensity, 0.0f, 1.35f);

        if (intensity <= 0.0001f)
            return;

        // Opacity itself is limited to one. Values above one are converted into an additional brightness boost in SetBrush.
        SetBrush(color, intensity);

        const float strokeWidth = radius > 0 ? (float)(radius * 2 + 1) : 1.0f;

        renderTarget->DrawLine(from, to, _Brush.Get(), strokeWidth, _StrokeStyle.Get());
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

        const float BlurRadius   = (float) radius * 1.45f;
        const float OuterRadius  = (float) radius * 1.10f;
        const float MiddleRadius = std::max(1.00f, (float) radius * 0.68f);
        const float InnerRadius  = std::max(0.75f, (float) radius * 0.28f);

        const D2D1_POINT_2F center = position;

        SetBrush(color, intensity * 0.055f);

        renderTarget->FillEllipse(D2D1::Ellipse(center, BlurRadius, BlurRadius), _Brush.Get());

        SetBrush(color, intensity * 0.13f);

        renderTarget->FillEllipse(D2D1::Ellipse(center, OuterRadius, OuterRadius), _Brush.Get());

        SetBrush(color, intensity * 0.42f);

        renderTarget->FillEllipse(D2D1::Ellipse(center, MiddleRadius, MiddleRadius), _Brush.Get());

        SetBrush(color, intensity * 1.12f);

        renderTarget->FillEllipse(D2D1::Ellipse(center, InnerRadius, InnerRadius), _Brush.Get());
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
    D2D1_COLOR_F effect_t::Colorize(size_t band, float energy, float particlePhase) const noexcept
    {
        const double BandAmount = (double) band / (double) (FrequencyBandCount - 1);
        const double Position   = Fraction((BandAmount * 0.88) + (particlePhase * 0.085) + (_AnimationTime * 0.0014));

        const auto GetColor = [&](Palette palette) noexcept
        {
            switch (palette)
            {
                case Palette::Cool:
                    return SamplePalette(CoolPalette, (float) Position);

                case Palette::Warm:
                default:
                    return SamplePalette(WarmPalette, (float) Position);
            }
        };

        const hsv_t PreviousColor = GetColor(_PreviousPalette);
        const hsv_t CurrentColor  = GetColor(_Palette);

        const float Amount = SmoothStep(0.f, 1.f, (float) _PaletteTransition);

        const hsv_t Color = InterpolateHSV(PreviousColor, CurrentColor, Amount);

        // Strong peaks gain saturation while quiet particles remain softer. The upper clamp avoids overly neon colors in dense clusters.
        // Value remains readable at low energy and receives restrained peak and transient boosts without flattening the palette highlights.
        const float H = InterpolateHue(Color.H, Fraction(Color.H + 0.012f), (float) Clamp(_SmoothedSpectralFlux * 0.45));
        const float S = (float) std::clamp(Color.S * (0.90 + energy * 0.13) + _SmoothedSpectralFlux * 0.025, 0.48, 0.92);
        const float V = std::clamp(Color.V * (0.86f + energy * 0.24f) + (float) ((_SmoothedRMS * 0.050) + (_SmoothedPeak * 0.025) + (_SmoothedSpectralFlux * 0.090)), 0.40f, 1.f);

        return HSVToColor(H, S, V);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::SetPalette(Palette palette) noexcept
    {
        if (_Palette == palette)
            return;

        _PreviousPalette = _Palette;
        _Palette = palette;
        _PaletteTransition = 0.0f;
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
            double DeltaTime = 0.;

            {
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, &Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
                });

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
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

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

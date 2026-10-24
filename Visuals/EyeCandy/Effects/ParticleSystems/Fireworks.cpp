
/** $VER: Fireworks.cpp (2026.10.19) **/

#include <pch.h>

#include "Fireworks.h"

#include <algorithm>
#include <format>
#include <cmath>

namespace Fireworks
{
    effect_t::effect_t()
    {
        _Flares.resize(24);
        _Sparks.resize(4096);

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
        return std::format(L"Fireworks, {} quality, {} speed, {} brightness, {} density",
            Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness], DensityNames[(size_t) _Density]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        for (auto & Flare : _Flares)
        {
            Flare.IsActive = false;
            Flare.TrailCount = 0;
        }

        for (auto & Spark : _Sparks)
            Spark.IsActive = false;

        _LaunchSequence = 0;
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

            _BackBufferRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

            ID2D1RenderTarget * BackBufferTarget = _BackBufferRenderTarget.Get();

            Fade(BackBufferTarget, deltaTime);
            RenderSky(BackBufferTarget);

            // Only persistent particles and idle stars are rendered into the fading back buffer. Flare trails and heads are transient.
            if (_HasAudio)
                RenderFireworks(BackBufferTarget);
            else
                RenderIdle(BackBufferTarget);

            if (FAILED(_BackBufferRenderTarget->EndDraw()))
            {
                DeleteDeviceDependentResources();
                return;
            }
        }

        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);

        // Draw after compositing so previous flare positions do not become separate dots in the persistent back buffer.
        if (_HasAudio)
            RenderFlareHeads(renderTarget);
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

        const audio_sample * Frames = chunk.get_data();

        const size_t SampleCount  = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();
        const double SampleRate   = (double) chunk.get_sample_rate();

        // Analyze only the most recent 1536 sample frames.
        constexpr size_t MaximumFrameCount = 1536;

        const size_t FrameCount = std::min<size_t>(SampleCount, MaximumFrameCount);

        if (FrameCount < 32)
            return;

        std::array<double, MaximumFrameCount> Samples;

        double SquaredSum = 0.;

        const size_t FirstFrame = SampleCount - FrameCount;
        const audio_sample * Frame = Frames + FirstFrame * ChannelCount;

        const double InverseChannelCount = 1.0 / (double) ChannelCount;

        // Prepares an efficient Hann-window generator.
        const double WindowStep = TwoPi / (double) (FrameCount - 1);
        const double WindowCos  = std::cos(WindowStep);
        const double WindowSin  = std::sin(WindowStep);

        double CurrentCos = 1.;
        double CurrentSin = 0.;

        for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
        {
            double Mono = 0.0;

            // Downmix the frame to mono.
            {
                const audio_sample * Channel    = Frame;
                const audio_sample * ChannelEnd = Channel + ChannelCount;

                while (Channel != ChannelEnd)
                    Mono += (double) *Channel++;

                Mono *= InverseChannelCount;
            }

            const double Sample = ClampAudio(Mono);
            const double Window = 0.5 - (0.5 * CurrentCos);

            Samples[FrameIndex] = Sample * Window;

            SquaredSum += Sample * Sample;

            _Peak = std::max(_Peak, std::abs(Sample));

            // Advance the Hann oscillator.
            {
                const double NextCosine = CurrentCos * WindowCos - CurrentSin * WindowSin;

                CurrentSin = (CurrentSin * WindowCos) + (CurrentCos * WindowSin);
                CurrentCos = NextCosine;
            }

            Frame += ChannelCount;
        }

        _RMS = std::sqrt(SquaredSum / (double) FrameCount);

        _HasAudio = (_Peak > 1.0e-5) || (_RMS > 1.0e-6);

        if (!_HasAudio)
            return;

        const double NyquistFrequency   = SampleRate * 0.5;
        const double InverseBandCount   = 1. / (double) (FrequencyBandCount - 1);

        constexpr double FrequencyRatio = 16'500. / 40.;            // 40 Hz to 16.5 kHz

        // Allocate the filter state.
        std::array<double, FrequencyBandCount> Coefficients;        // Goertzel-style resonators
        std::array<double, FrequencyBandCount> Previous1 {};
        std::array<double, FrequencyBandCount> Previous2 {};

        // Select logarithmically spaced center frequencies. More bands are allocated to lower frequencies.
        for (size_t Band = 0; Band < FrequencyBandCount; ++Band)
        {
            const double Amount = (double) Band * InverseBandCount; // [0, 1]

            const double CenterFrequency = 40. * std::pow(FrequencyRatio, Amount);
            const double Frequency       = std::clamp(CenterFrequency, 20., NyquistFrequency * 0.95);

            Coefficients[Band] = 2. * std::cos(TwoPi * Frequency / SampleRate);
        }

        // Run all frequency filters (bank of second-order Goertzel-style filters)
        for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
        {
            const double Sample = Samples[FrameIndex];

            for (size_t Band = 0; Band < FrequencyBandCount; ++Band)
            {
                const double Current = Sample + Coefficients[Band] * Previous1[Band] - Previous2[Band];

                Previous2[Band] = Previous1[Band];
                Previous1[Band] = Current;
            }
        }

        // Convert the filter states into magnitudes.
        const double InverseFrameCount = 1. / (double) FrameCount;
        const double LogarithmicScale  = 1. / std::log1p(100.);

        double MaximumEnergy = 0.;

        for (size_t Band = 0; Band < FrequencyBandCount; ++Band)
        {
            const double Amount      = (double) Band * InverseBandCount;
            const double Coefficient = Coefficients[Band];

            const double Power     = std::max(0.0, Previous2[Band] * Previous2[Band] + Previous1[Band] * Previous1[Band] - Coefficient * Previous1[Band] * Previous2[Band]);
            const double Magnitude = std::sqrt(Power) * InverseFrameCount;

            // Compress and shape the band energy. Slightly suppress low bands and increasingly boosts high bands.
            _BandEnergy[Band] = Clamp(std::log1p(Magnitude * 100.0) * LogarithmicScale * Lerp(0.90, 1.50, Amount));

            MaximumEnergy = std::max(MaximumEnergy, _BandEnergy[Band]);
        }

        // Normalize the spectrum.
        if (MaximumEnergy > 1.0e-5)
        {
            const double Normalization = 1. / std::max(0.3, MaximumEnergy);

            for (auto & Energy : _BandEnergy)
                Energy = Clamp(Energy * Normalization);
        }

        // Calculate the spectral flux.
        {
            for (size_t Band = 0; Band < FrequencyBandCount; ++Band)
            {
                _SpectralFlux += std::max(0., _BandEnergy[Band] - _PreviousBandEnergy[Band]);

                _PreviousBandEnergy[Band] = _BandEnergy[Band]; // Remember the band energy for the next chunk.
            }

            _SpectralFlux = Clamp(_SpectralFlux / (double) FrequencyBandCount * 4.); // Boost the spectral flux artificially by 4 to make it react more strongly.
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime)
    {
        const double SpeedFactor    = (_Speed == EyeCandy::Speed::Slow) ? 0.65 : (_Speed == EyeCandy::Speed::Fast) ? 1.55 : 1.0;
        const double AnimationDelta = deltaTime * SpeedFactor;

        _AnimationTime += AnimationDelta;

        _SmoothedPeak         = ExponentialSmoothing(_SmoothedPeak,         _Peak,         deltaTime, (_Peak         > _SmoothedPeak)         ? 36.f : 7.f);
        _SmoothedRMS          = ExponentialSmoothing(_SmoothedRMS,          _RMS,          deltaTime, (_RMS          > _SmoothedRMS)          ? 24.f : 5.f);
        _SmoothedSpectralFlux = ExponentialSmoothing(_SmoothedSpectralFlux, _SpectralFlux, deltaTime, (_SpectralFlux > _SmoothedSpectralFlux) ? 42.f : 8.f);

        for (size_t i = 0; i < FrequencyBandCount; ++i)
        {
            _SmoothedBandEnergy[i] = ExponentialSmoothing(_SmoothedBandEnergy[i], _HasAudio ? _BandEnergy[i] : 0.f, deltaTime, 18.f);
            _BandCooldown[i]       = std::max(0., _BandCooldown[i] - AnimationDelta);
        }

        _BassCooldown = std::max(0., _BassCooldown - AnimationDelta);

        LaunchFlares(AnimationDelta);
        UpdateFlares(AnimationDelta);
        GenerateWork(AnimationDelta);

        _SkyLight       = ExponentialSmoothing(_SkyLight, _SkyLightTarget, deltaTime, _SkyLightTarget > _SkyLight ? 14.0f : 2.6f);
        _SkyLightTarget = ExponentialSmoothing(_SkyLightTarget, 0.0f, deltaTime, 3.8f);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::LaunchFlares(double deltaTime)
    {
        if (!_HasAudio)
            return;

        // The density multiplier affects the continuous launch rate, the probability of per-band launches and the duration of per-band cooldowns.
        const double DensityMultiplier = (_Density == Density::Low) ? 0.82 : (_Density == Density::High) ? 1.90 : 1.28;

        // Mechanism 1: Accumulate launch credit over time; guarantees ongoing activity.
        // - A base rate of 0.50 causes some activity even with relatively quiet audio.
        // - Loudness: RMS represents broader, sustained signal power. Louder passages fill the accumulator more quickly.
        // - Spectral flux: Attacks, transitions, percussion, and changing instrumentation can significantly accelerate the launch rate.
        {
            _LaunchAccumulator += deltaTime * (0.50 + (_SmoothedRMS * 5.) + (_SmoothedSpectralFlux * 7.)) * DensityMultiplier;

            if (_LaunchAccumulator >= 1.)
            {
                // Launch from the strongest band.
                size_t StrongestBand = 0;

                for (size_t i = 1; i < FrequencyBandCount; ++i)
                {
                    if (_BandEnergy[i] > _BandEnergy[StrongestBand])
                        StrongestBand = i;
                }

                const double FlareEnergy = 0.5 + (_BandEnergy[StrongestBand] * 0.65);

                LaunchFlare(StrongestBand, Clamp(FlareEnergy));

                // Reset the accumulator.
                _LaunchAccumulator -= 1.;
            }
        }

        // Mechanism 2: Synchronized bass bursts of 3 or 4 flares.
        // - Calculate the weighted bass energy. The lowest band has the largest weight.
        // - Calculate a smoothed bass baseline.
        // - Calculate the impact of the bass.
        {
            const double BassEnergy = (_BandEnergy[0] * 0.48) + (_BandEnergy[1] * 0.34) + (_BandEnergy[2] * 0.18);
            const double BassBaseline = (_SmoothedBandEnergy[0] * 0.48) + (_SmoothedBandEnergy[1] * 0.34) + (_SmoothedBandEnergy[2] * 0.18);
            const double BassImpact = std::max(0., BassEnergy - BassBaseline * 0.70) + (_SmoothedPeak * 0.24);

            // The Absolute-energy condition prevents a large relative change in a very quiet signal from triggering a burst.
            // The bass impact condition prevents sustained bass from repeatedly triggering solely because it remains loud.
            if ((_BassCooldown <= 0.) && (BassEnergy > 0.25) && (BassImpact > 0.095))
            {
                // Launch 3 or 4 bass flares.
                const size_t BurstCount = (BassImpact > 0.42) ? 4 : 3;

                for (size_t i = 0; i < BurstCount; ++i)
                {
                    const double FlareEnergy = BassEnergy + BassImpact * 0.65;

                    LaunchFlare(i % 3, Clamp(FlareEnergy));
                }

                _BassCooldown   = Lerp(0.33, 0.58, 1. - Clamp(BassImpact));
                _SkyLightTarget = std::clamp(_SkyLightTarget + BassImpact * 0.16, 0., 0.82);
            }
        }

        // Mechanism 3: Detailed reactions to individual spectral events.
        {
            for (size_t i = 0; i < FrequencyBandCount; ++i)
            {
                const double FlareEnergy = _BandEnergy[i];

                const double Rise        = std::max(0., FlareEnergy - (_SmoothedBandEnergy[i] * 0.78));
                const double Trigger     = 0.095 - (_SmoothedSpectralFlux * 0.035);
                const double Probability = Clamp(((Rise - Trigger) * 25.) + (_SpectralFlux * 0.24)) * DensityMultiplier * (deltaTime * 60.);

                if ((_BandCooldown[i] <= 0.) && (FlareEnergy > 0.18) && (Random(_RandomState) < Probability))
                {
                    LaunchFlare(i, FlareEnergy);

                    _BandCooldown[i] = Lerp(0.16, 0.48, Random(_RandomState)) / DensityMultiplier;
                }
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::LaunchFlare(size_t band, double energy) noexcept
    {
        auto Iterator = std::find_if(_Flares.begin(), _Flares.end(), [](const flare_t & flare) noexcept
        {
            return !flare.IsActive;
        });

        if (Iterator == _Flares.end())
            return;

        flare_t & Flare = *Iterator;

        // Mutating the persistent RNG guarantees a distinct identity for several launches of the same band during one update.
        Random(_RandomState);

        uint32_t State = Hash(_RandomState ^ ((uint32_t)band * 0x9E3779B9u) ^ (_LaunchSequence * 0x85EBCA6Bu));

        const double BandAmount = (double) band / (double) (FrequencyBandCount - 1);

        // A rotating set of lanes prevents the bass-heavy lower bands from concentrating launches in the left half of the display. Random jitter inside each lane keeps the pattern organic.
        constexpr size_t LaneCount = 8;

        const size_t RandomOffset = std::min(LaneCount - 1, (size_t)(Random(State) * (double)LaneCount));

        const size_t Lane = ((size_t) _LaunchSequence + RandomOffset) % LaneCount;

        ++_LaunchSequence;

        const double LaneWidth        = 1. / (double)LaneCount;
        const double HorizontalAmount = ((double)Lane + Lerp(0.12, 0.88, Random(State))) * LaneWidth;

        Flare.Position =
        {
            (float) (_Width * HorizontalAmount),
            (float)  _Height
        };

        Flare.Previous = Flare.Position;

        const double DirectionToCenter = std::clamp(0.5 - HorizontalAmount, -0.5, 0.5);

        Flare.Velocity =
        {
            (float) ( _Width  * ((DirectionToCenter * 0.045) + (RandomSigned(State) * 0.040))),
            (float) (-_Height * Lerp(0.48, 0.76, energy))
        };

        Flare.Age    = 0.;
        Flare.Fuse   = Lerp(0.70, 1.35, Random(State));
        Flare.Energy = std::clamp(0.48 + (energy * 0.72) + (_SmoothedSpectralFlux * 0.35), 0., 1.35);
        Flare.Hue    = Fraction((BandAmount * 0.88) + (Random(State) * 0.28) + (_AnimationTime * 0.025));
        Flare.Band   = band;
        Flare.Seed   = State;

        Flare.TrailCount = 1;

        Flare.Trail[0].Position = Flare.Position;
        Flare.Trail[0].Age = 0.;

        Flare.IsActive = true;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateFlares(double deltaTime)
    {
        for (flare_t & Flare : _Flares)
        {
            if (!Flare.IsActive)
                continue;

            Flare.Previous    = Flare.Position;
            Flare.Age        += deltaTime;

            Flare.Velocity.y += (float) (_Height * 0.31 * deltaTime);
            Flare.Velocity.x *= (float) std::exp(-deltaTime * 0.35);

            Flare.Position.x += (float) (Flare.Velocity.x * deltaTime);
            Flare.Position.y += (float) (Flare.Velocity.y * deltaTime);

            UpdateFlareTrail(Flare, deltaTime);

            if ((Flare.Age >= Flare.Fuse) || (Flare.Velocity.y > (-_Height * 0.035)))
                ExplodeFlare(Flare);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateFlareTrail(flare_t & flare, double deltaTime) noexcept
    {
        for (size_t i = 0; i < flare.TrailCount; ++i)
            flare.Trail[i].Age += deltaTime;

        size_t FirstValidPoint = 0;

        while ((FirstValidPoint < flare.TrailCount) && (flare.Trail[FirstValidPoint].Age >= FlareTrailLifetime))
            ++FirstValidPoint;

        if (FirstValidPoint > 0)
        {
            const size_t RemainingPointCount = flare.TrailCount - FirstValidPoint;

            for (size_t i = 0; i < RemainingPointCount; ++i)
                flare.Trail[i] = flare.Trail[i + FirstValidPoint];

            flare.TrailCount = RemainingPointCount;
        }

        if (flare.TrailCount > 0)
        {
            const D2D1_POINT_2F & LastPosition = flare.Trail[flare.TrailCount - 1].Position;

            const double DeltaX = (double)flare.Position.x - LastPosition.x;
            const double DeltaY = (double)flare.Position.y - LastPosition.y;

            if (((DeltaX * DeltaX) + (DeltaY * DeltaY)) < 1.0)
                return;
        }

        if (flare.TrailCount == FlareTrailPointCount)
        {
            for (size_t i = 1; i < flare.TrailCount; ++i)
                flare.Trail[i - 1] = flare.Trail[i];

            --flare.TrailCount;
        }

        flare_trail_point_t & Point = flare.Trail[flare.TrailCount++];

        Point.Position = flare.Position;
        Point.Age = 0.;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderFireworks(ID2D1RenderTarget * renderTarget) noexcept
    {
        // Rising flares are intentionally omitted here. This render target is persistent, so drawing them here would retain every old head position.
        for (const spark_t & Spark : _Sparks)
        {
            if (!Spark.IsActive)
                continue;

            const double Life      = Clamp(Spark.Age / std::max(Spark.Lifetime, 0.001));
            const double Fade      = (1. - Life) * (1. - Life);
            const double Twinkle   = 0.78 + (0.22 * std::sin(_AnimationTime * 21. + (Spark.Seed * 0.001)));
            const double Intensity = Spark.Brightness * Fade * Twinkle;

            const uint32_t Color = HSVToBGRA((float) Spark.Hue, (float) Spark.Saturation, (float) Clamp(0.68 + (Intensity * 0.42)));

            DrawGlowLine(renderTarget, Spark.Previous, Spark.Position, Color, (float)(Intensity * 0.68), 0.70f);

            const float Radius = (Life < 0.15) ? 3.f : 2.f;

            DrawGlowPoint(renderTarget, Spark.Position, Color, (float)Intensity, Radius);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderFlareHeads(ID2D1RenderTarget * renderTarget) noexcept
    {
        for (const flare_t & Flare : _Flares)
        {
            if (!Flare.IsActive)
                continue;

            RenderFlareTrail(renderTarget, Flare);

            const uint32_t Color = HSVToBGRA((float)Flare.Hue, 0.82f, 1.f); 

            const float Intensity = (float)std::clamp(0.82 + (Flare.Energy * 0.28), 0., 1.25);

            SetBrush(_ForegroundBrush.Get(), Color, Intensity * 0.06f);

            renderTarget->FillEllipse(D2D1::Ellipse(Flare.Position,  6.6f,  6.6f), _ForegroundBrush.Get());

            SetBrush(_ForegroundBrush.Get(), Color, Intensity * 0.24f);

            renderTarget->FillEllipse(D2D1::Ellipse(Flare.Position, 3.45f, 3.45f), _ForegroundBrush.Get());

            SetBrush(_ForegroundBrush.Get(), 0xFFFFFFFFu, Intensity);

            renderTarget->FillEllipse(D2D1::Ellipse(Flare.Position, 1.1f, 1.1f), _ForegroundBrush.Get());
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderFlareTrail(ID2D1RenderTarget * renderTarget, const flare_t & Flare) noexcept
    {
        if (Flare.TrailCount == 0)
            return;

        const uint32_t Color = HSVToBGRA((float) Flare.Hue, 0.78f, 1.f);

        D2D1_POINT_2F To = Flare.Position;

        for (size_t ReverseIndex = Flare.TrailCount; ReverseIndex > 0; --ReverseIndex)
        {
            const flare_trail_point_t & Point = Flare.Trail[ReverseIndex - 1];

            const double Life       = Clamp(1. - (Point.Age / FlareTrailLifetime));
            const double SmoothLife = Life * Life * (3. - (2. * Life));

            const float Intensity = (float)(SmoothLife * (0.38 + Flare.Energy * 0.22));
            const float Width     = (float)Lerp(0.45, 1.35, SmoothLife);

            DrawForegroundGlowLine(renderTarget, Point.Position, To, Color, Intensity, Width);

            To = Point.Position;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderIdle(ID2D1RenderTarget * renderTarget) noexcept
    {
        constexpr size_t Stars = 48;

        for (size_t i = 0; i < Stars; ++i)
        {
            uint32_t State = Hash((uint32_t)i * 0x9E3779B9u);

            const D2D1_POINT_2F Position =
            {
                (float) (Random(State) * _Width),
                (float) (Random(State) * _Height * 0.78)
            };

            const double Frequency = Lerp(0.8, 2.4, Random(State));
            const double Phase     = Random(State) * TwoPi;

            double Twinkle = 0.5 + (0.5 * std::sin((_AnimationTime * Frequency) + Phase));

            Twinkle *= Twinkle;

            const float Intensity = (float) Lerp(0.08, 0.48, Twinkle);
            const float Radius    = (float) Lerp(1.35, 2.25, Random(State));

            DrawGlowPoint(renderTarget, Position, 0xFFDDE8FFu, Intensity, Radius);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::ExplodeFlare(flare_t & flare) noexcept
    {
        const size_t SparkCountBase = (_Quality == EyeCandy::Quality::Low) ? 58u : (_Quality == EyeCandy::Quality::High) ? 176u : 108u;
        const double DensityFactor = (_Density == Density::Low ? 0.82 : _Density == Density::High ? 1.72 : 1.22);

        const double BassWeight = 1. - ((double)flare.Band / (double)(FrequencyBandCount - 1));
        const double BassBurst = (flare.Band < 3) ? Lerp(1.25, 1.62, BassWeight) : 1.;

        const size_t SparkCount = (size_t)std::lround((double)SparkCountBase * DensityFactor * BassBurst * Lerp(0.82, 1.34, std::min(flare.Energy, 1.)));

        uint32_t State = flare.Seed;

        const double PatternChoice = Random(State);

        int Pattern;

        if (PatternChoice < 0.25)
            Pattern = 0;       // 25% filled burst
        else
        if (PatternChoice < 0.43)
            Pattern = 1;       // 18% shell
        else
        if (PatternChoice < 0.56)
            Pattern = 2;       // 13% spokes
        else
        if (PatternChoice < 0.66)
            Pattern = 3;       // 10% butterfly
        else
        if (PatternChoice < 0.75)
            Pattern = 4;       //  9% double ring
        else
        if (PatternChoice < 0.82)
            Pattern = 5;       //  7% spiral
        else
        if (PatternChoice < 0.89)
            Pattern = 6;       //  7% palm
        else
        if (PatternChoice < 0.95)
            Pattern = 7;       //  6% willow
        else
        if (PatternChoice < 0.98)
            Pattern = 8;       //  3% ellipse
        else
            Pattern = 9;       //  2% star

        for (size_t i = 0; i < SparkCount; ++i)
        {
            spark_t * Spark = AcquireSpark();

            if (!Spark)
                break;

            const double Amount = (double)i / (double)std::max<size_t>(SparkCount, 1);

            double Angle      = Amount * TwoPi;
            double Radius     = std::sqrt(Random(State));
            double HueSpread  = 0.055;
            double SpeedScale = 1.;
            double ScaleX     = 1.;
            double ScaleY    = 1.;

            double LifetimeMinimum = 0.75;
            double LifetimeMaximum = 2.35;

            double DragMinimum = 0.58;
            double DragMaximum = 1.45;

            double GravityMinimum = 0.65;
            double GravityMaximum = 1.25;

            switch (Pattern)
            {
                default:
                case 0:
                {
                    // Filled chrysanthemum: an evenly distributed circular explosion.
                    Angle += RandomSigned(State) * 0.065;
                    Radius = std::sqrt(Random(State));
                    break;
                }

                case 1:
                {
                    // Hollow shell: nearly every spark travels at the same speed.
                    Angle += RandomSigned(State) * 0.045;
                    Radius = 0.82 + (Random(State) * 0.18);
                    break;
                }

                case 2:
                {
                    // Radial spokes: sparks are concentrated into ten directions.
                    constexpr size_t SpokeCount = 10;

                    Angle = ((double)(i % SpokeCount) / (double)SpokeCount) * TwoPi + RandomSigned(State) * 0.10;
                    Radius = Lerp(0.42, 1., Random(State));
                    HueSpread = 0.32;
                    break;
                }

                case 3:
                {
                    // Four-lobed butterfly.
                    Angle += RandomSigned(State) * 0.045;
                    Radius = 0.40 + (0.60 * std::abs(std::sin(Angle * 2.)));
                    break;
                }

                case 4:
                {
                    // Double ring: alternating sparks form an inner and outer shell.
                    Angle += RandomSigned(State) * 0.035;

                    const bool IsOuterRing = ((i & 1u) == 0u);

                    Radius = IsOuterRing ? Lerp(0.88, 1.00, Random(State)) : Lerp(0.48, 0.58, Random(State));
                    HueSpread = IsOuterRing ? 0.035 : 0.12;
                    break;
                }

                case 5:
                {
                    // Spiral: radius increases over several revolutions.
                    constexpr double Turns = 3.5;

                    Angle = (Amount * TwoPi * Turns) + RandomSigned(State) * 0.055;
                    Radius = 0.24 + (0.76 * std::sqrt(Amount));
                    HueSpread = 0.20;
                    break;
                }

                case 6:
                {
                    // Palm: a narrow upward fan with long, heavy branches.
                    constexpr double Upward = -TwoPi * 0.25;

                    Angle = Upward + RandomSigned(State) * 0.58;
                    Radius = Lerp(0.58, 1., std::sqrt(Random(State)));

                    SpeedScale = 1.08;

                    LifetimeMinimum = 1.35;
                    LifetimeMaximum = 2.85;

                    DragMinimum = 0.34;
                    DragMaximum = 0.72;

                    GravityMinimum = 1.05;
                    GravityMaximum = 1.55;

                    HueSpread = 0.025;
                    break;
                }

                case 7:
                {
                    // Willow: slow, long-lived sparks that droop under gravity.
                    Angle += RandomSigned(State) * 0.055;
                    Radius = Lerp(0.55, 1., Random(State));

                    SpeedScale = 0.78;

                    LifetimeMinimum = 1.85;
                    LifetimeMaximum = 3.60;

                    DragMinimum = 0.82;
                    DragMaximum = 1.30;

                    GravityMinimum = 1.15;
                    GravityMaximum = 1.75;

                    HueSpread = 0.025;
                    break;
                }

                case 8:
                {
                    // Elliptical or Saturn-like shell.
                    Angle += RandomSigned(State) * 0.035;
                    Radius = Lerp(0.82, 1., Random(State));

                    ScaleX = 1.18;
                    ScaleY = 0.48;

                    HueSpread = 0.065;
                    break;
                }

                case 9:
                {
                    // Five-point star. cos(5 * angle) creates five radial lobes. The absolute value keeps the radius positive.
                    Angle += RandomSigned(State) * 0.025;

                    const double StarShape = 0.5 + (0.5 * std::cos(Angle * 5.));

                    Radius = Lerp(0.42, 1., StarShape) * Lerp(0.92, 1.08, Random(State));
                    HueSpread = 0.10;
                    break;
                }
            }

            const double Speed = std::min(_Width, _Height) * Lerp(0.16, 0.47, Radius) * flare.Energy * SpeedScale;

            Spark->Position = flare.Position;
            Spark->Previous = flare.Position;

            Spark->Velocity =
            {
                (float)(std::cos(Angle) * Speed * ScaleX + flare.Velocity.x * 0.18),
                (float)(std::sin(Angle) * Speed * ScaleY + flare.Velocity.y * 0.10)
            };

            Spark->Age        = 0.;
            Spark->Lifetime   = Lerp(LifetimeMinimum, LifetimeMaximum, Random(State)) * Lerp(0.85, 1.20, flare.Energy);

            Spark->Hue        = Fraction(flare.Hue + RandomSigned(State) * HueSpread);
            Spark->Saturation = Lerp(0.72, 1.00, Random(State));
            Spark->Brightness = Lerp(0.78, 1.25, Random(State));

            Spark->Drag       = Lerp(DragMinimum, DragMaximum, Random(State));
            Spark->Gravity    = Lerp(GravityMinimum, GravityMaximum, Random(State));
            Spark->Seed       = State;
            Spark->IsActive   = true;
        }

        _SkyHue = flare.Hue;

        const double Crowding = Clamp((double) SparkCount / 260.);

        _SkyLightTarget = std::clamp(_SkyLightTarget + (flare.Energy * 0.105) + (Crowding * 0.115), 0., 0.82);

        flare.IsActive = false;
    }

    /// <summary>
    /// Finds the first spark that is inactive.
    /// </summary>
    effect_t::spark_t * effect_t::AcquireSpark() noexcept
    {
        for (spark_t & Spark : _Sparks)
        {
            if (!Spark.IsActive)
                return &Spark;
        }

        return nullptr;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::UpdateSpark(size_t index, double deltaTime) noexcept
    {
        spark_t & Spark = _Sparks[index];

        if (!Spark.IsActive)
            return;

        Spark.Previous = Spark.Position;
        Spark.Age += deltaTime;

        if (Spark.Age >= Spark.Lifetime)
        {
            Spark.IsActive = false;

            return;
        }

        Spark.Velocity.y += (float)(_Height * 0.22 * Spark.Gravity * deltaTime);

        const float Drag = (float)std::exp(-deltaTime * Spark.Drag);

        Spark.Velocity.x *= Drag;
        Spark.Velocity.y *= Drag;

        Spark.Position.x += Spark.Velocity.x * (float)deltaTime;
        Spark.Position.y += Spark.Velocity.y * (float)deltaTime;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StartWorkers()
    {
        const auto WorkerCount = (size_t)std::max(1u, std::thread::hardware_concurrency());

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

                DeltaTime = _SparkDeltaTime;

                Generation = _Generation;
            }

            for (;;)
            {
                const size_t SparkIndex = _NextSpark.fetch_add(1, std::memory_order_relaxed);

                if (SparkIndex >= _Sparks.size())
                    break;

                UpdateSpark(SparkIndex, DeltaTime);
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
        if (_Sparks.empty() || _Workers.empty())
            return;

        {
            const std::lock_guard Lock(_WorkerMutex);

            _SparkDeltaTime = deltaTime;

            _NextSpark.store(0, std::memory_order_relaxed);

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
    void effect_t::Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept
    {
        const double Persistence = 0.885 + (_SmoothedRMS * 0.025);
        const double Decay       = std::pow(std::clamp(Persistence, 0.78, 0.955), (deltaTime * 60.));

        const auto Opacity = (float) Clamp(1. - Decay);

        _Brush->SetColor(D2D1::ColorF(0., 0., 0., Opacity));
        _Brush->SetOpacity(1.f);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderSky(ID2D1RenderTarget * renderTarget) noexcept
    {
        if (_SkyLight <= 0.002)
            return;

        const uint32_t Color = HSVToBGRA((float)_SkyHue, 0.64f, 0.60f);

        const double ShapedLight = _SkyLight * _SkyLight;

        const float Opacity = (float)((_SkyLight * 0.032) + (ShapedLight * 0.060));

        SetBrush(_Brush.Get(), Color, Opacity);

        renderTarget->FillRectangle(_Rectangle, _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float width) noexcept
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

        Radius = std::max(1, (int)std::lround(width) + Radius);

        DrawLine(renderTarget, from, to, color, intensity * 0.055f, Radius * 2);
        DrawLine(renderTarget, from, to, color, intensity * 0.180f, Radius);
        DrawLine(renderTarget, from, to, color, intensity * 0.920f, 0);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawForegroundGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float width) noexcept
    {
        intensity = std::clamp(intensity, 0.f, 1.f);

        DrawForegroundLine(renderTarget, from, to, color, intensity * 0.07f, width * 4.5f);
        DrawForegroundLine(renderTarget, from, to, color, intensity * 0.22f, width * 2.2f);
        DrawForegroundLine(renderTarget, from, to, color, intensity,         width);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, int radius) noexcept
    {
        SetBrush(_Brush.Get(), color, intensity);

        auto StrokeWidth = (radius > 0) ? (float)((radius * 2) + 1) : 1.f;

        renderTarget->DrawLine(from, to, _Brush.Get(), StrokeWidth, _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawForegroundLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float opacity, float width) noexcept
    {
        if ((opacity <= 0.f) || (width <= 0.f))
            return;

        SetBrush(_ForegroundBrush.Get(), color, opacity);

        renderTarget->DrawLine(from, to, _ForegroundBrush.Get(), width, _StrokeStyle.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawGlowPoint(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F position, uint32_t color, float intensity, float radius) noexcept
    {
        SetBrush(_Brush.Get(), color, intensity * 0.06f);

        auto Radius = radius * 2.20f;

        renderTarget->FillEllipse(D2D1::Ellipse(position, Radius, Radius), _Brush.Get());

        SetBrush(_Brush.Get(), color, intensity * 0.24f);

        Radius = radius * 1.15f;

        renderTarget->FillEllipse(D2D1::Ellipse(position, Radius, Radius), _Brush.Get());

        SetBrush(_Brush.Get(), color, intensity);

        Radius = std::max(0.75f, radius * 0.34f);

        renderTarget->FillEllipse(D2D1::Ellipse(position, Radius, Radius), _Brush.Get());
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetBrush(ID2D1SolidColorBrush * brush, uint32_t color, float opacity) noexcept
    {
        const float BrightnessFactor = [&]() noexcept
        {
            switch (_Brightness)
            {
                case Brightness::Low:
                    return 0.78f;

                default:
                case Brightness::Medium:
                    return 1.00f;

                case Brightness::High:
                    return 1.28f;
            }
        }();

        brush->SetColor(D2D1::ColorF
        (
            std::clamp((float) GetChannel(color, 16u) / 255.f * BrightnessFactor, 0.f, 1.f),
            std::clamp((float) GetChannel(color,  8u) / 255.f * BrightnessFactor, 0.f, 1.f),
            std::clamp((float) GetChannel(color,  0u) / 255.f * BrightnessFactor, 0.f, 1.f),
                       (float) GetChannel(color, 24u) / 255.f)
        );

        brush->SetOpacity(std::clamp(opacity, 0.f, 1.f));
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr =__super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        if ((_RenderTarget == renderTarget) && _BackBufferRenderTarget && _BackBuffer && _Brush && _ForegroundBrush && _StrokeStyle)
        {
            const D2D1_SIZE_U Size = _BackBufferRenderTarget->GetPixelSize();

            if ((Size.width == (UINT32) _Width) && (Size.height == (UINT32) _Height))
                return S_OK;
        }

        {
            hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0., 0., 0., 1.), _Brush.ReleaseAndGetAddressOf());

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

            _BackBufferRenderTarget->Clear(D2D1::ColorF(0., 0., 0., 1.));

            hr = _BackBufferRenderTarget->EndDraw();

            if (FAILED(hr))
                return hr;

            hr = _BackBufferRenderTarget->GetBitmap(_BackBuffer.ReleaseAndGetAddressOf());

            if (FAILED(hr))
                return hr;
        }

        {
            hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0., 0., 0., 1.), _ForegroundBrush.ReleaseAndGetAddressOf());

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

        _ForegroundBrush.Reset();
        _StrokeStyle.Reset();
        _Brush.Reset();

        _BackBuffer.Reset();
        _BackBufferRenderTarget.Reset();
    }
}

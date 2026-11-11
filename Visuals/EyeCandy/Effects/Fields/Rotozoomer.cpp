
/** $VER: Rotozoomer.cpp (2026.11.07) **/

#include <pch.h>

#include "Rotozoomer.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Rotozoomer
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
        return std::format(L"Rotozoomer, {} quality, {} speed, {} brightness", Qualities[(size_t) _Quality], Speeds[(size_t) _Speed], Brightnesses[(size_t) _Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _RotationVelocity  = 0.035;
        _RotationCS        = 1.;
        _RotationSN        = 0.;

        _ColorPhase        = 0.;

        _SmoothedDeltaTime = 1. / 60.;

        _ZoomPhase    = 0.;
        _ZoomVelocity = 0.18;
        _ZoomResponse = 0.18;

        _WarpPhase     = 0.;
        _PreviousPulse = 0.;
        _TextureTime   = 0.;

        for (auto & Band : _Bands)
        {
            Band.Energy         = 0.;
            Band.SmoothedEnergy = 0.;
            Band.PreviousEnergy = 0.;
        }
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

        Prepare(deltaTime);
        Update(deltaTime);

        RenderTexture();

        // Render the bitmap.
        {
            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _Stride);

            renderTarget->DrawBitmap(_BackBuffer.Get(), _ViewPort);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _HasAudio = false;

        _Peak         = 0.;
        _RMS          = 0.;
        _SpectralFlux = 0.;

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
        const size_t MaxFrames = std::min<size_t>(FrameCount, 1536);

        if (MaxFrames < 32)
            return;

        if (_Samples.size() < MaxFrames)
            _Samples.resize(MaxFrames);

        auto Samples = std::span<double>(_Samples.data(), MaxFrames);

        // Initialize the energy accumulators.
        double Sum  = 0.;           // Full-audio band

        {
            const double InverseChannelCount = 1. / (double) ChannelCount;
            const double WindowStep          = TwoPi / ((double) MaxFrames - 1.);

            for (size_t i = 0; i < MaxFrames; ++i)
            {
                const audio_sample * Frame = Frames + (i * ChannelCount);

                double Mono = 0.;

                for (size_t c = 0; c < ChannelCount; ++c)
                    Mono += Frame[c];

                Mono = ClampAudio(Mono * InverseChannelCount);

                // Apply a Hann window.
                const double Window = .5 - std::cos((double) i * WindowStep) * .5;

                Samples[i] = Mono * Window;

                _Peak = std::max(_Peak, std::abs(Mono));

                Sum += Mono * Mono;
            }
        }

        _RMS = std::sqrt(Sum / (double) MaxFrames);

        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);

        if (!_HasAudio)
            return;

        // Perform a spectrum analysis of logarithmically spaced frequency bands.
        {
            const double NyquistFrequency = SampleRate * 0.5;
            const double MaximumFrequency = std::min(HiFrequency, NyquistFrequency);

            constexpr double TuningConstant = 90.;
            const double LogarithmicScale  = 1. / std::log1p(TuningConstant);

            double MaximumEnergy = 0.;

            for (size_t i = 0; i < BandCount; ++i)
            {
                const double BandAmount = (double) i * InverseBandCount; // [0, 1]

                double BandPower = 0.;

                {
                    const double Frequency = LoFrequency * std::pow(MaximumFrequency / LoFrequency, BandAmount);

                    // Configure the Goertzel detector.
                    const double Omega       = TwoPi * Frequency / SampleRate;
                    const double Coefficient = 2. * std::cos(Omega);

                    double q1 = 0., q2 = 0.;

                    for (double Sample : Samples)
                    {
                        const double q0 = Sample + Coefficient * q1 - q2;

                        q2 = q1;
                        q1 = q0;
                    }

                    // Convert the detector state to power.
                    BandPower = std::max(0., q2 * q2 + q1 * q1 - Coefficient * q1 * q2);
                }

                const double Magnitude = std::sqrt(BandPower) / (double) MaxFrames;

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
    void effect_t::Prepare(double deltaTime) noexcept
    {
        _SmoothedPeak         = ExponentialSmoothing(_SmoothedPeak,         _Peak,         deltaTime, (_Peak         > _SmoothedPeak)         ? 24. : 5.);
        _SmoothedRMS          = ExponentialSmoothing(_SmoothedRMS,          _RMS,          deltaTime, (_RMS          > _SmoothedRMS)          ? 20. : 4.);
        _SmoothedSpectralFlux = ExponentialSmoothing(_SmoothedSpectralFlux, _SpectralFlux, deltaTime, (_SpectralFlux > _SmoothedSpectralFlux) ? 34. : 6.);

        {
            for (auto & Band : _Bands)
                Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, Band.Energy > Band.SmoothedEnergy ? 22. : 4.5);

            _BassLevel   = (_Bands[0].SmoothedEnergy + _Bands[1].SmoothedEnergy + _Bands[ 2].SmoothedEnergy + _Bands[ 3].SmoothedEnergy) * .25;
            _MidLevel    = (_Bands[4].SmoothedEnergy + _Bands[5].SmoothedEnergy + _Bands[ 6].SmoothedEnergy + _Bands[ 7].SmoothedEnergy) * .25;
            _TrebleLevel = (_Bands[8].SmoothedEnergy + _Bands[9].SmoothedEnergy + _Bands[10].SmoothedEnergy + _Bands[11].SmoothedEnergy) * .25;
        }

        _PreviousPulse = std::max(_PreviousPulse * std::exp(-deltaTime * 7.), _SpectralFlux);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = [&]() noexcept
        {
            switch (_Speed)
            {
                case Speed::Slow:
                    return 0.55;

                default:
                case Speed::Normal:
                    return 1.0;

                case Speed::Fast:
                    return 1.75;
            }
        }();

        _AnimationTime += deltaTime * SpeedFactor;
        _TextureTime   += deltaTime;

        const double TargetZoomVelocity = _HasAudio ? (.12 + _BassLevel * 1.65 + _PreviousPulse * .45) : .08;
        const double ResponseRate = (TargetZoomVelocity > _ZoomResponse) ? 8. : 2.5;

        _ZoomResponse = ExponentialSmoothing(_ZoomResponse, TargetZoomVelocity, deltaTime, ResponseRate);
        _ZoomVelocity = ExponentialSmoothing(_ZoomVelocity, _ZoomResponse, deltaTime, 4.);

        const double ZoomDelta = std::min(deltaTime, 1. / 30.) * _ZoomVelocity * SpeedFactor;

        _ZoomPhase += ZoomDelta;

        const double TargetRotationVelocity = (.035 + _MidLevel * .075 + _PreviousPulse * .025) * SpeedFactor;

        _RotationVelocity = ExponentialSmoothing(_RotationVelocity, TargetRotationVelocity, deltaTime, 3.5);

        const double ClampedDeltaTime = std::clamp(deltaTime, 1. / 240., 1. / 30.);

        _SmoothedDeltaTime = ExponentialSmoothing(_SmoothedDeltaTime, ClampedDeltaTime, deltaTime, 10.);

        const double RotationDelta = _SmoothedDeltaTime * _RotationVelocity * TwoPi;

        const double DeltaCS = std::cos(RotationDelta);
        const double DeltaSN = std::sin(RotationDelta);

        const double RotationCS = _RotationCS * DeltaCS - _RotationSN * DeltaSN;
        const double RotationSN = _RotationSN * DeltaCS + _RotationCS * DeltaSN;

        const double RotationLength = std::sqrt(RotationCS * RotationCS + RotationSN * RotationSN);

        _RotationCS = RotationCS / RotationLength;
        _RotationSN = RotationSN / RotationLength;

        _ColorPhase = Fraction(_ColorPhase + _SmoothedDeltaTime * _RotationVelocity * .035);
        _WarpPhase  = Fraction(_WarpPhase + deltaTime * (.12 + _TrebleLevel * .48) * SpeedFactor);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderTexture() noexcept
    {
        if (_Pixels.empty() || _RasterWidth == 0 || _RasterHeight == 0)
            return;

        _RenderCS = _RotationCS;
        _RenderSN = _RotationSN;

        _RenderZoomFraction = Fraction(_ZoomPhase);
        _RenderZoom         = std::pow(2., _RenderZoomFraction);

        _RenderDetailLevel = (int64_t) std::floor(_ZoomPhase);
        _RenderAspect      = (double) _RasterWidth / (double) _RasterHeight;
        _RenderWarp        = (_HasAudio ? .012 : .003) + _TrebleLevel * .055 + _SmoothedSpectralFlux * .018;
        _RenderPhase       = _WarpPhase * TwoPi;

        constexpr double TextureDuration    = 3.;
        constexpr double TransitionDuration = 1.2;
        constexpr size_t TextureCount       = 82;
        constexpr size_t PaletteCount       = 8;

        const double TexturePosition = _TextureTime / TextureDuration;
        const size_t TextureIndex    = (size_t) std::floor(TexturePosition);
        const double TextureFraction = Fraction(TexturePosition);
        const double BlendStart      = 1. - TransitionDuration / TextureDuration;

        _RenderTextureA     =  TextureIndex      % TextureCount;
        _RenderTextureB     = (TextureIndex + 1) % TextureCount;
        _RenderPaletteA     =  TextureIndex      % PaletteCount;
        _RenderPaletteB     = (TextureIndex + 1) % PaletteCount;
        _RenderTextureBlend = SmoothStep(BlendStart, 1., TextureFraction);
        _RenderPaletteBlend = _RenderTextureBlend;

        if (_Workers.empty())
        {
            RenderRows(0, _RasterHeight);
            return;
        }

        {
            std::lock_guard Lock(_WorkerMutex);

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
    /// Start the worker threads.
    /// </summary>
    void effect_t::StartWorkers()
    {
        const size_t HardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 1, 1, _MaxWorkers);

        _Workers.reserve(WorkerCount);

        try
        {
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

        for (auto & Worker : _Workers)
        {
            if (Worker.joinable())
                Worker.join();
        }

        _Workers.clear();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Worker(size_t workerIndex) noexcept
    {
        size_t Generation = 0;

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

            const UINT32 Begin = (UINT32) ((uint64_t) _RasterHeight * workerIndex       / _Workers.size());
            const UINT32 End   = (UINT32) ((uint64_t) _RasterHeight * (workerIndex + 1) / _Workers.size());

            RenderRows(Begin, End);

            {
                std::lock_guard Lock(_WorkerMutex);
                if (--_PendingWorkers == 0)
                    _CompletionCondition.notify_one();
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderRows(UINT32 begin, UINT32 end) noexcept
    {
        const double InverseHeight = 1. / (double) _RasterHeight;
        const double PixelStep     = _RenderAspect / (double) _RasterWidth;

        const double UStep = PixelStep * _RenderCS;
        const double VStep = PixelStep * _RenderSN;

        const double PhaseU1 = _RenderPhase;
        const double PhaseU2 = -_RenderPhase * 1.37;

        const double PhaseV1 = -_RenderPhase * 1.13;
        const double PhaseV2 =  _RenderPhase * .83;

        const double StartPX = (.5 / (double) _RasterWidth - .5) * _RenderAspect;

        for (UINT32 y = begin; y < end; ++y)
        {
            const double py = ((double) y + .5) * InverseHeight - .5;

            double BaseU = StartPX * _RenderCS - py * _RenderSN;
            double BaseV = StartPX * _RenderSN + py * _RenderCS;

            uint32_t * Pixel = _Pixels.data() + (size_t) y * _RasterWidth;

            const uint32_t * const EndPixel = Pixel + _RasterWidth;

            while (Pixel < EndPixel)
            {
                double u = BaseU;
                double v = BaseV;

                const double WaveU = std::sin(v * 31. + PhaseU1) + std::sin(v * 73. + PhaseU2) * .35;
                const double WaveV = std::cos(u * 27. + PhaseV1) + std::sin(u * 61. + PhaseV2) * .35;

                u += WaveU * _RenderWarp;
                v += WaveV * _RenderWarp;

                *Pixel++ = SampleTexture(u, v, _RenderPhase, _RenderDetailLevel, _RenderZoomFraction);

                BaseU += UStep;
                BaseV += VStep;
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::SamplePattern(size_t texture, double u, double v, double phase, double random, int64_t x, int64_t y, uint64_t hash) const noexcept
    {
        const double cu     = u - .5;
        const double cv     = v - .5;
        const double Radius = std::sqrt(cu * cu + cv * cv);
        const double Angle  = std::atan2(cv, cu);

        switch (texture)
        {
            case 0:
            {
                const double Edge   = std::max(std::abs(cu), std::abs(cv));
                const double Ring   = .5 + .5 * std::sin((u * u + v * v) * (18. + random * 16.) + phase + random * TwoPi);
                const double Stripe = .5 + .5 * std::sin((u - v) * TwoPi * ( 2. + random * 3. ) - phase * .7);
                const double Cell   = ((x ^ y ^ (int64_t)(hash >> 32)) & 1) ? .72 : .16;

                return Clamp(Cell + (Ring * .19) + (Stripe * .11) + (Edge > .455 ? .23 : 0.));
            }

            case 1:
                return Clamp((.5 + .5 * std::sin(Radius * TwoPi * (8. + random * 5.) - phase * 1.3)) * .62 +
                             (.5 + .5 * std::sin(Angle * (6. + (double) (hash & 3)) + phase)) * .38);

            case 2:
            {
                const double A = .5 + .5 * std::sin((v + std::sin(u * TwoPi + phase) * .09) * TwoPi * 5.);
                const double B = .5 + .5 * std::sin((u + std::cos(v * TwoPi - phase) * .09) * TwoPi * 5.);

                return Clamp(A * B * 1.25 + (1. - A) * (1. - B) * .35);
            }

            case 3:
            {
                const double Plasma = std::sin((u + phase * .035) * TwoPi * 2.) +
                                      std::sin((v - phase * .027) * TwoPi * 3.) +
                                      std::sin((u + v) * TwoPi * 2. + phase * .4) +
                                      std::cos(Radius * TwoPi * 7. - phase);

                return Clamp(.5 + (Plasma * .125));
            }

            case 4:
            {
                const double Tracks = 1. - std::min(std::abs(std::sin(u * TwoPi * 4.)), std::abs(std::sin(v * TwoPi * 4.)));
                const double Nodes = std::exp(-(cu * cu + cv * cv) * (70. + (random * 40.)));

                return Clamp(Tracks * .72 + Nodes * .55 + random * .08);
            }

            case 5:
                return Clamp((.5 + .5 * std::sin((std::abs(cu) - cv) * TwoPi * (5. + random * 2.) + phase * .45)) * .58 +
                             (.5 + .5 * std::cos((std::abs(cu) + std::abs(cv)) * TwoPi * 7. - phase * .8)) * .42);

            case 6:
            {
                const double HexX = std::abs(Fraction(u * 3.) - .5);
                const double HexY = std::abs(Fraction(v * 3. + std::floor(u * 3.) * .5) - .5);

                return Clamp(1. - std::min(HexX * 1.35 + HexY * .65, HexY * 1.5) * 2.2 + (.5 + .5 * std::sin(phase + random * TwoPi)) * .18);
            }

            case 7:
            {
                const double Spiral = .5 + .5 * std::sin(Angle  * 5. + std::log(Radius + .035) * 10. - phase);
                const double Rings  = .5 + .5 * std::cos(Radius * TwoPi * 12. + phase * .6);

                return Clamp(Spiral * .7 + Rings * .3);
            }

            case 8:
            {
                const double Voronoi = std::min(std::hypot(u - random,                  v - Fraction(random * 7.31)),
                                                std::hypot(u - Fraction(random * 3.17), v - random));

                return Clamp(1. - Voronoi * 2.4 + .15 * std::sin(phase + random * TwoPi));
            }

            case 9:
            {
                const double BrickU = Fraction(u * 4. + (double) (y & 1) * .5);
                const double BrickV = Fraction(v * 6.);
                const double Mortar = std::min(std::min(BrickU, 1. - BrickU), std::min(BrickV, 1. - BrickV));

                return Clamp(Mortar * 8. + random * .16);
            }

            case 10:
            {
                const double Wave = std::sin((u + std::sin(v * TwoPi * 2. + phase) * .12) * TwoPi * 7.);
                const double Echo = std::cos((v + std::sin(u * TwoPi * 3. - phase) * .08) * TwoPi * 5.);

                return Clamp(.5 + Wave * .32 + Echo * .18);
            }

            case 11:
            {
                const double Star = .5 + .5 * std::sin(Angle * (8. + (double) (hash & 3)) + Radius * TwoPi * 9. - phase);
                const double Halo = std::exp(-Radius * (4. + random * 3.));

                return Clamp(Star * .64 + Halo * .55);
            }

            case 12: return .5 + .5 * std::sin((u + v) * TwoPi * 6. + phase);
            case 13: return .5 + .5 * std::sin((u - v) * TwoPi * 8. - phase * .7);
            case 14: return .5 + .5 * std::sin((std::abs(cu) + std::abs(cv)) * TwoPi * 9. - phase);
            case 15: return .5 + .5 * std::sin(std::max(std::abs(cu), std::abs(cv)) * TwoPi * 12. + phase);
            case 16: return ((x + y) & 1) ? .82 : .18;
            case 17: return ((x ^ y) & 1) ? .76 : .24;
            case 18: return (Fraction((u + phase * .025) * 8.) < .32) ? .88 : .16;
            case 19: return (Fraction((v - phase * .02) * 10.) < .24) ? .92 : .12;
            case 20: return (std::max(std::abs(Fraction(u * 6.) - .5), std::abs(Fraction(v * 6.) - .5)) > .42) ? .9 : .14;
            case 21:
            {
                const double du = Fraction(u * 6.) - .5, dv = Fraction(v * 6.) - .5;

                return Clamp(1. - (du * du + dv * dv) * 10.);
            }
            case 22: return .5 + .5 * std::sin((u * 3. + v * 7.) * TwoPi + phase);
            case 23: return .5 + .5 * std::sin((u * 9. - v * 2.) * TwoPi - phase);
            case 24: return std::floor(Fraction((u + v + phase * .02) * 6.) * 4.) / 3.;
            case 25: return (Fraction((std::abs(cu) - cv + phase * .015) * 8.) < .5) ? .8 : .2;
            case 26: return .5 + .5 * std::sin((std::abs(cu) * 2. + cv) * TwoPi * 7. + phase);
            case 27: return (Fraction(u * 5.) + Fraction(v * 5.) < 1.) ? .84 : .16;
            case 28: return (std::min(std::abs(Fraction(u * 5.) - .5), std::abs(Fraction(v * 5.) - .5)) < .11) ? .9 : .12;
            case 29: return std::clamp(1. - std::abs(cu + cv) * 4., 0., 1.);
            case 30: return std::clamp(1. - std::abs(cu - cv) * 4., 0., 1.);
            case 31: return (Fraction(Angle / TwoPi + Radius * 5. - phase * .08) < .5) ? .86 : .14;

            case 32: return .5 + .5 * std::sin(u * TwoPi * 4. + phase);
            case 33: return .5 + .5 * std::sin(v * TwoPi * 4. - phase);
            case 34: return .5 + .5 * std::sin((u + v * .5) * TwoPi * 7. + phase);
            case 35: return .5 + .5 * std::sin((v - u * .5) * TwoPi * 7. - phase);
            case 36: return .5 + .5 * std::sin((u * 2. + v * 3.) * TwoPi + phase * .6);
            case 37: return .5 + .5 * std::sin((u * 5. - v * 4.) * TwoPi - phase * .8);
            case 38: return (Fraction((u + phase * .01) * 12.) < .5) ? .78 : .22;
            case 39: return (Fraction((v - phase * .012) * 12.) < .5) ? .78 : .22;
            case 40: return (Fraction((u + v) * 9. + phase * .02) < .5) ? .84 : .16;
            case 41: return (Fraction((u - v) * 9. - phase * .02) < .5) ? .84 : .16;
            case 42: return std::floor(Fraction(u * 8. + phase * .01) * 3.) * .5;
            case 43: return std::floor(Fraction(v * 8. - phase * .01) * 3.) * .5;
            case 44: return std::floor(Fraction((u + v) * 7.) * 4.) / 3.;
            case 45: return std::floor(Fraction((u - v) * 7.) * 4.) / 3.;
            case 46: return std::clamp(1. - std::abs(Fraction(u * 7.) - .5) * 4., 0., 1.);
            case 47: return std::clamp(1. - std::abs(Fraction(v * 7.) - .5) * 4., 0., 1.);
            case 48: return std::clamp(1. - std::abs(Fraction((u + v) * 6.) - .5) * 4., 0., 1.);
            case 49: return std::clamp(1. - std::abs(Fraction((u - v) * 6.) - .5) * 4., 0., 1.);
            case 50: return (std::abs(Fraction(u * 6.) - .5) < .18 || std::abs(Fraction(v * 6.) - .5) < .18) ? .88 : .12;
            case 51: return (std::abs(Fraction(u * 8.) - .5) < .1 && std::abs(Fraction(v * 8.) - .5) < .1) ? .94 : .1;
            case 52: return (std::abs(Fraction(u * 5.) - .5) + std::abs(Fraction(v * 5.) - .5) < .32) ? .9 : .1;
            case 53: return (std::max(std::abs(Fraction(u * 5.) - .5), std::abs(Fraction(v * 5.) - .5)) < .28) ? .86 : .14;
            case 54: return (Fraction(u * 6.) < Fraction(v * 6.)) ? .82 : .18;
            case 55: return (Fraction(u * 7.) + Fraction(v * 7.) < .65) ? .9 : .1;
            case 56: return (Fraction(u * 7.) + Fraction(v * 7.) > 1.35) ? .9 : .1;
            case 57: return ((x + y + (int64_t)(phase * 2.)) % 3 == 0) ? .88 : .14;
            case 58: return ((x - y + (int64_t)(phase * 2.)) & 3) ? .18 : .9;
            case 59: return ((x * 3 + y * 5) & 1) ? .8 : .2;
            case 60: return ((x & 1) == (y & 1)) ? .86 : .14;
            case 61: return .5 + .5 * std::sin(Radius * TwoPi * 6. + phase);
            case 62: return .5 + .5 * std::sin(Radius * TwoPi * 11. - phase * .7);
            case 63: return (Fraction(Radius * 12. - phase * .04) < .5) ? .84 : .16;
            case 64: return std::floor(Fraction(Radius * 9. + phase * .03) * 4.) / 3.;
            case 65: return .5 + .5 * std::sin(Angle * 6. + phase);
            case 66: return .5 + .5 * std::sin(Angle * 10. - phase * .8);
            case 67: return (Fraction(Angle / TwoPi * 8. + phase * .03) < .5) ? .86 : .14;
            case 68: return (Fraction(Angle / TwoPi * 12. - phase * .02) < .32) ? .92 : .12;
            case 69: return .5 + .5 * std::sin(Angle * 5. + Radius * TwoPi * 4. - phase);
            case 70: return .5 + .5 * std::sin(Angle * 7. - Radius * TwoPi * 5. + phase);
            case 71: return (Fraction(Angle / TwoPi * 6. + Radius * 4. - phase * .04) < .5) ? .86 : .14;
            case 72: return (Fraction(Angle / TwoPi * 9. - Radius * 3. + phase * .03) < .4) ? .9 : .1;
            case 73: return std::clamp(1. - std::abs(std::abs(cu) - std::abs(cv)) * 7., 0., 1.);
            case 74: return std::clamp(1. - std::abs(std::max(std::abs(cu), std::abs(cv)) - .28) * 8., 0., 1.);
            case 75: return std::clamp(1. - std::abs(std::abs(cu) + std::abs(cv) - .42) * 7., 0., 1.);
            case 76: return (std::abs(cu) < .12 || std::abs(cv) < .12) ? .9 : .12;
            case 77: return (std::abs(cu + cv) < .12 || std::abs(cu - cv) < .12) ? .9 : .12;
            case 78: return std::clamp((cu * cu + cv * cv) * 5., 0., 1.);
            case 79: return std::clamp(1. - (cu * cu + cv * cv) * 5., 0., 1.);
            case 80: return std::clamp(.5 + (cu * cv) * 5., 0., 1.);
            case 81: return ((hash >> 16) & 1) ? (.62 + random * .32) : (.38 - random * .32);

            default: return .5;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::SamplePalette(size_t palette, double amount) const noexcept
    {
        amount = Clamp(amount);

        switch (palette)
        {
            case 0:  return HSVToColor((float) Fraction(.56 + amount * .22), .92f, (float) amount);
            case 1:  return HSVToColor((float) Fraction(.96 + amount * .13), .88f, (float) std::pow(amount, .82));
            case 2:  return HSVToColor((float) Fraction(.11 + amount * .08), .95f, (float) amount);
            case 3:  return HSVToColor((float) Fraction(.34 + amount * .19), .82f, (float) std::pow(amount, .9));
            case 4:  return HSVToColor((float) Fraction(.74 + amount * .18), .86f, (float) amount);
            case 5:  return D2D1::ColorF((float)  amount,        (float) (amount * .78 + .05), (float) (amount * .32 + .02));
            case 6:  return D2D1::ColorF((float) (amount * .22), (float) (amount * .90),       (float) (amount * .78));
            default: return D2D1::ColorF((float) std::pow(amount, .75), (float) std::pow(amount, 1.35), (float) std::pow(amount, 2.1));
        }
    }

    /// <summary>
    /// 
    /// </summary>
    uint32_t effect_t::SampleTexture(double u, double v, double phase, int64_t detailLevel, double zoomFraction) const noexcept
    {
        const double ContinuousDepth = (double) detailLevel + zoomFraction;

        double Pattern = 0.;
        double Weight  = 0.;

        const bool SampleTextureA = _RenderTextureBlend <      1.e-6;
        const bool SampleTextureB = _RenderTextureBlend > 1. - 1.e-6;

        for (int RelativeOctave = -2; RelativeOctave <= 4; ++RelativeOctave)
        {
            const int64_t GlobalOctave = detailLevel + RelativeOctave;
            const double VisualOctave  = (double) GlobalOctave - ContinuousDepth;
            const double Envelope      = SmoothStep(-1.5, -.25, VisualOctave) * (1. - SmoothStep(3.25, 4.5, VisualOctave));

            if (Envelope <= 1.e-6)
                continue;

            const double OctaveWeight = Envelope * std::exp2(-.72 * std::max(0., VisualOctave));
            const double Scale        = 8.       * std::exp2(VisualOctave);

            const double su = u * Scale;
            const double sv = v * Scale;

            const double fu = Fraction(su);
            const double fv = Fraction(sv);

            const int64_t cx = (int64_t) std::floor(su);
            const int64_t cy = (int64_t) std::floor(sv);

            uint64_t Hash = (uint64_t) (cx * 0x632be59bd9b4e019LL) ^ (uint64_t) (cy * 0x9e3779b97f4a7c15LL);

            Hash ^= (uint64_t) GlobalOctave * 0x94d049bb133111ebULL;
            Hash ^= Hash >> 30;
            Hash *= 0xbf58476d1ce4e5b9ULL;
            Hash ^= Hash >> 27;

            const double Random = (double) (Hash & 0xFFFFu) / 65535.;

            double Value;

            if (SampleTextureA)
                Value = SamplePattern(_RenderTextureA, fu, fv, phase, Random, cx, cy, Hash);
            else
            if (SampleTextureB)
                Value = SamplePattern(_RenderTextureB, fu, fv, phase, Random, cx, cy, Hash);
            else
            {
                const double A = SamplePattern(_RenderTextureA, fu, fv, phase, Random, cx, cy, Hash);
                const double B = SamplePattern(_RenderTextureB, fu, fv, phase, Random, cx, cy, Hash);

                Value = Lerp(A, B, _RenderTextureBlend);
            }

            Pattern += Value * OctaveWeight;
             Weight += OctaveWeight;
        }

        Pattern = Clamp(Pattern / std::max(Weight, 1.e-6));

        const double PaletteAmount = Clamp(Pattern * (.86 + _TrebleLevel * .14));

        D2D1_COLOR_F Color;

        if (SampleTextureA)
            Color = SamplePalette(_RenderPaletteA, PaletteAmount);
        else
        if (SampleTextureB)
            Color = SamplePalette(_RenderPaletteB, PaletteAmount);
        else
        {
            const D2D1_COLOR_F A = SamplePalette(_RenderPaletteA, PaletteAmount);
            const D2D1_COLOR_F B = SamplePalette(_RenderPaletteB, PaletteAmount);

            Color = D2D1::ColorF((float) Lerp((double) A.r, (double) B.r, _RenderPaletteBlend),
                                 (float) Lerp((double) A.g, (double) B.g, _RenderPaletteBlend),
                                 (float) Lerp((double) A.b, (double) B.b, _RenderPaletteBlend));
        }

        const double BrightnessFactor = (_Brightness == Brightness::Low)  ? 0.72 : (_Brightness == Brightness::High) ? 1.18 : 0.94;

        return PackColor(Color.r * BrightnessFactor, Color.g * BrightnessFactor, Color.b * BrightnessFactor);
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        const double Resolution = (_Quality == Quality::Low) ? .34 : (_Quality == Quality::High) ? .67 : .50;

        const UINT32 Width  = std::max<UINT32>(64, (UINT32) std::ceil(_Width  * Resolution));
        const UINT32 Height = std::max<UINT32>(64, (UINT32) std::ceil(_Height * Resolution));

        if (_RenderTarget == renderTarget && _BackBuffer && (Width == _RasterWidth) && (Height == _RasterHeight))
            return S_OK;

        DeleteDeviceDependentResources();

        _RasterWidth  = Width;
        _RasterHeight = Height;

        _Stride = _RasterWidth * BytesPerPixel;

        _Pixels.assign((size_t) Width * Height, 0xFF000000u);

        const auto Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.f, 96.f);

        hr = renderTarget->CreateBitmap(D2D1::SizeU(Width, Height), _Pixels.data(), Width * sizeof(uint32_t), Properties, _BackBuffer.ReleaseAndGetAddressOf());

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
        _RenderTarget = nullptr;

        _BackBuffer.Reset();

        _Pixels.clear();

        _RasterWidth  = 0;
        _RasterHeight = 0;

        __super::DeleteDeviceDependentResources();
    }
}

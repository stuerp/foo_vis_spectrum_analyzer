
/** $VER: Rotozoomer.cpp (2026.11.02) */

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
        return std::format(L"Rotozoomer, {} quality, {} speed, {} brightness", Qualities[(size_t)_Quality], Speeds[(size_t)_Speed], Brightnesses[(size_t)_Brightness]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _RotationVelocity = .035;
        _RotationCS = 1.;
        _RotationSN = 0.;
        _ColorPhase = 0.;
        _SmoothedDeltaTime = 1. / 60.;
        _ZoomPhase = 0.;
        _ZoomVelocity = .18;
        _ZoomResponse = .18;
        _WarpPhase = 0.;
        _PreviousPulse = 0.;
        _TextureTime = 0.;
        _BandEnergy.fill(0.);
        _SmoothedBandEnergy.fill(0.);
        _PreviousBandEnergy.fill(0.);
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
        RenderTexture();
        if (!_Bitmap || _Pixels.empty())
            return;
        const D2D1_RECT_U Source = D2D1::RectU(0, 0, _PixelWidth, _PixelHeight);
        hr = _Bitmap->CopyFromMemory(&Source, _Pixels.data(), _PixelWidth * sizeof(uint32_t));
        if (FAILED(hr))
        {
            DeleteDeviceDependentResources();
            return;
        }
        renderTarget->DrawBitmap(_Bitmap.Get(), _Rectangle, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk)
    {
        _Peak = 0.;
        _RMS = 0.;
        _SpectralFlux = 0.;
        _BandEnergy.fill(0.);
        _HasAudio = false;
        if (chunk.is_empty())
            return;
        const size_t FrameCount = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();
        const size_t SampleRate = chunk.get_sample_rate();
        const size_t MaxFrames = std::min<size_t>(FrameCount, 1536);
        if (MaxFrames < 32 || ChannelCount == 0 || SampleRate == 0)
            return;
        if (_Samples.size() < MaxFrames)
            _Samples.resize(MaxFrames);
        auto Samples = std::span<double>(_Samples.data(), MaxFrames);
        double Sum = 0.;
        const audio_sample * Frames = chunk.get_data();
        for (size_t i = 0; i < MaxFrames; ++i)
        {
            const audio_sample * Frame = Frames + (i * ChannelCount);
            double Mono = 0.;
            for (size_t c = 0; c < ChannelCount; ++c)
                Mono += Frame[c];
            Mono = ClampAudio(Mono / (double)ChannelCount);
            const double Window = .5 - std::cos(TwoPi * (double)i / ((double)MaxFrames - 1.)) * .5;
            Samples[i] = Mono * Window;
            _Peak = std::max(_Peak, std::abs(Mono));
            Sum += Mono * Mono;
        }
        _RMS = std::sqrt(Sum / (double)MaxFrames);
        _HasAudio = (_Peak > 1.e-5) || (_RMS > 1.e-6);
        if (!_HasAudio)
            return;
        const double Nyquist = (double)SampleRate * .5;
        double MaximumEnergy = 0.;
        for (size_t i = 0; i < BandCount; ++i)
        {
            const double Amount = (double)i / (double)(BandCount - 1);
            const double Frequency = std::clamp(40. * std::pow(400., Amount), 20., Nyquist * .95);
            const double Omega = TwoPi * Frequency / (double)SampleRate;
            const double Coefficient = 2. * std::cos(Omega);
            double q1 = 0., q2 = 0.;
            for (double Sample : Samples)
            {
                const double q0 = Sample + Coefficient * q1 - q2;
                q2 = q1;
                q1 = q0;
            }
            const double Power = std::max(0., q2 * q2 + q1 * q1 - Coefficient * q1 * q2);
            const double Magnitude = std::sqrt(Power) / (double)MaxFrames;
            _BandEnergy[i] = Clamp(std::log1p(Magnitude * 90.) / std::log1p(90.) * Lerp(.95, 1.55, Amount));
            MaximumEnergy = std::max(MaximumEnergy, _BandEnergy[i]);
        }
        if (MaximumEnergy > 1.e-5)
        {
            for (double & Energy : _BandEnergy)
                Energy = Clamp(Energy / std::max(.28, MaximumEnergy));
        }
        for (size_t i = 0; i < BandCount; ++i)
        {
            _SpectralFlux += std::max(0., _BandEnergy[i] - _PreviousBandEnergy[i]);
            _PreviousBandEnergy[i] = _BandEnergy[i];
        }
        _SpectralFlux = Clamp(_SpectralFlux / (double)BandCount * 5.5);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        const double SpeedFactor = (_Speed == Speed::Slow) ? .55 : (_Speed == Speed::Fast) ? 1.75 : 1.;
        _AnimationTime += deltaTime * SpeedFactor;
        _TextureTime += deltaTime;
        _SmoothedPeak = ExponentialSmoothing(_SmoothedPeak, _Peak, deltaTime, (_Peak > _SmoothedPeak) ? 24. : 5.);
        _SmoothedRMS = ExponentialSmoothing(_SmoothedRMS, _RMS, deltaTime, (_RMS > _SmoothedRMS) ? 20. : 4.);
        _SmoothedSpectralFlux = ExponentialSmoothing(_SmoothedSpectralFlux, _SpectralFlux, deltaTime, (_SpectralFlux > _SmoothedSpectralFlux) ? 34. : 6.);
        for (size_t i = 0; i < BandCount; ++i)
            _SmoothedBandEnergy[i] = ExponentialSmoothing(_SmoothedBandEnergy[i], _HasAudio ? _BandEnergy[i] : 0., deltaTime, _BandEnergy[i] > _SmoothedBandEnergy[i] ? 22. : 4.5);
        _BassEnergy = (_SmoothedBandEnergy[0] + _SmoothedBandEnergy[1] + _SmoothedBandEnergy[2] + _SmoothedBandEnergy[3]) * .25;
        _MidEnergy = (_SmoothedBandEnergy[4] + _SmoothedBandEnergy[5] + _SmoothedBandEnergy[6] + _SmoothedBandEnergy[7]) * .25;
        _TrebleEnergy = (_SmoothedBandEnergy[8] + _SmoothedBandEnergy[9] + _SmoothedBandEnergy[10] + _SmoothedBandEnergy[11]) * .25;
        const double Pulse = std::max(_PreviousPulse * std::exp(-deltaTime * 7.), _SpectralFlux);
        const double TargetZoomVelocity = _HasAudio ? (.12 + _BassEnergy * 1.65 + Pulse * .45) : .08;
        const double ResponseRate = (TargetZoomVelocity > _ZoomResponse) ? 8. : 2.5;
        _ZoomResponse = ExponentialSmoothing(_ZoomResponse, TargetZoomVelocity, deltaTime, ResponseRate);
        _ZoomVelocity = ExponentialSmoothing(_ZoomVelocity, _ZoomResponse, deltaTime, 4.);
        const double ZoomDelta = std::min(deltaTime, 1. / 30.) * _ZoomVelocity * SpeedFactor;
        _ZoomPhase += ZoomDelta;
        const double TargetRotationVelocity = (.035 + _MidEnergy * .075 + Pulse * .025) * SpeedFactor;
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
        _WarpPhase = Fraction(_WarpPhase + deltaTime * (.12 + _TrebleEnergy * .48) * SpeedFactor);
        _PreviousPulse = Pulse;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderTexture() noexcept
    {
        if (_Pixels.empty() || _PixelWidth == 0 || _PixelHeight == 0)
            return;
        _RenderCS = _RotationCS;
        _RenderSN = _RotationSN;
        _RenderZoomFraction = Fraction(_ZoomPhase);
        _RenderZoom = std::pow(2., _RenderZoomFraction);
        _RenderDetailLevel = (int64_t)std::floor(_ZoomPhase);
        _RenderAspect = (double)_PixelWidth / (double)_PixelHeight;
        _RenderWarp = (_HasAudio ? .012 : .003) + _TrebleEnergy * .055 + _SmoothedSpectralFlux * .018;
        _RenderPhase = _WarpPhase * TwoPi;
        constexpr double TextureDuration = 3.;
        constexpr double TransitionDuration = 1.2;
        constexpr size_t TextureCount = 12;
        constexpr size_t PaletteCount = 8;
        const double TexturePosition = _TextureTime / TextureDuration;
        const size_t TextureIndex = (size_t)std::floor(TexturePosition);
        const double TextureFraction = Fraction(TexturePosition);
        const double BlendStart = 1. - TransitionDuration / TextureDuration;
        _RenderTextureA = TextureIndex % TextureCount;
        _RenderTextureB = (TextureIndex + 1) % TextureCount;
        _RenderPaletteA = TextureIndex % PaletteCount;
        _RenderPaletteB = (TextureIndex + 1) % PaletteCount;
        _RenderTextureBlend = SmoothStep(BlendStart, 1., TextureFraction);
        _RenderPaletteBlend = _RenderTextureBlend;
        if (_Workers.empty())
        {
            RenderRows(0, _PixelHeight);
            return;
        }
        {
            std::lock_guard Lock(_WorkerMutex);
            _PendingWorkers = _Workers.size();
            ++_Generation;
        }
        _WorkerCondition.notify_all();
        std::unique_lock Lock(_WorkerMutex);
        _CompletionCondition.wait(Lock, [this]() noexcept
        {
            return _PendingWorkers == 0;
        });
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StartWorkers()
    {
        const unsigned int HardwareThreads = std::thread::hardware_concurrency();
        const size_t WorkerCount = std::clamp<size_t>(HardwareThreads > 1 ? HardwareThreads - 1 : 0, 0, 8);
        _Workers.reserve(WorkerCount);
        for (size_t i = 0; i < WorkerCount; ++i)
            _Workers.emplace_back(&effect_t::WorkerLoop, this, i);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::StopWorkers() noexcept
    {
        {
            std::lock_guard Lock(_WorkerMutex);
            _IsStopping = true;
            ++_Generation;
        }
        _WorkerCondition.notify_all();
        for (std::thread & Worker : _Workers)
        {
            if (Worker.joinable())
                Worker.join();
        }
        _Workers.clear();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::WorkerLoop(size_t workerIndex) noexcept
    {
        size_t Generation = 0;
        for (;;)
        {
            {
                std::unique_lock Lock(_WorkerMutex);
                _WorkerCondition.wait(Lock, [this, Generation]() noexcept
                {
                    return _IsStopping || _Generation != Generation;
                });
                if (_IsStopping)
                    return;
                Generation = _Generation;
            }
            const UINT32 Begin = (UINT32)((uint64_t)_PixelHeight * workerIndex / _Workers.size());
            const UINT32 End = (UINT32)((uint64_t)_PixelHeight * (workerIndex + 1) / _Workers.size());
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
        for (UINT32 y = begin; y < end; ++y)
        {
            const double py = ((double)y + .5) / (double)_PixelHeight - .5;
            uint32_t * Pixel = _Pixels.data() + (size_t)y * _PixelWidth;
            for (UINT32 x = 0; x < _PixelWidth; ++x)
            {
                const double px = (((double)x + .5) / (double)_PixelWidth - .5) * _RenderAspect;
                double u = px * _RenderCS - py * _RenderSN;
                double v = px * _RenderSN + py * _RenderCS;
                const double WaveU = std::sin(v * 31. + _RenderPhase) + std::sin(v * 73. - _RenderPhase * 1.37) * .35;
                const double WaveV = std::cos(u * 27. - _RenderPhase * 1.13) + std::sin(u * 61. + _RenderPhase * .83) * .35;
                u += WaveU * _RenderWarp;
                v += WaveV * _RenderWarp;
                *Pixel++ = SampleTexture(u, v, _RenderPhase, _RenderDetailLevel, _RenderZoomFraction);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    double effect_t::SamplePattern(size_t texture, double u, double v, double phase, double random, int64_t x, int64_t y, uint64_t hash) const noexcept
    {
        const double cu = u - .5;
        const double cv = v - .5;
        const double Radius = std::sqrt(cu * cu + cv * cv);
        const double Angle = std::atan2(cv, cu);
        switch (texture)
        {
            case 0:
            {
                const double Edge = std::max(std::abs(cu), std::abs(cv));
                const double Ring = .5 + .5 * std::sin((u * u + v * v) * (18. + random * 16.) + phase + random * TwoPi);
                const double Stripe = .5 + .5 * std::sin((u - v) * TwoPi * (2. + random * 3.) - phase * .7);
                const double Cell = ((x ^ y ^ (int64_t)(hash >> 32)) & 1) ? .72 : .16;
                return std::clamp(Cell + Ring * .19 + Stripe * .11 + (Edge > .455 ? .23 : 0.), 0., 1.);
            }
            case 1:
                return std::clamp((.5 + .5 * std::sin(Radius * TwoPi * (8. + random * 5.) - phase * 1.3)) * .62 + (.5 + .5 * std::sin(Angle * (6. + (double) (hash & 3)) + phase)) * .38, 0., 1.);
            case 2:
            {
                const double A = .5 + .5 * std::sin((v + std::sin(u * TwoPi + phase) * .09) * TwoPi * 5.);
                const double B = .5 + .5 * std::sin((u + std::cos(v * TwoPi - phase) * .09) * TwoPi * 5.);
                return std::clamp(A * B * 1.25 + (1. - A) * (1. - B) * .35, 0., 1.);
            }
            case 3:
            {
                const double Plasma = std::sin((u + phase * .035) * TwoPi * 2.) + std::sin((v - phase * .027) * TwoPi * 3.) + std::sin((u + v) * TwoPi * 2. + phase * .4) + std::cos(Radius * TwoPi * 7. - phase);
                return std::clamp(.5 + Plasma * .125, 0., 1.);
            }
            case 4:
            {
                const double Tracks = 1. - std::min(std::abs(std::sin(u * TwoPi * 4.)), std::abs(std::sin(v * TwoPi * 4.)));
                const double Nodes = std::exp(-(cu * cu + cv * cv) * (70. + random * 40.));
                return std::clamp(Tracks * .72 + Nodes * .55 + random * .08, 0., 1.);
            }
            case 5:
                return std::clamp((.5 + .5 * std::sin((std::abs(cu) - cv) * TwoPi * (5. + random * 2.) + phase * .45)) * .58 + (.5 + .5 * std::cos((std::abs(cu) + std::abs(cv)) * TwoPi * 7. - phase * .8)) * .42, 0., 1.);
            case 6:
            {
                const double HexX = std::abs(Fraction(u * 3.) - .5);
                const double HexY = std::abs(Fraction(v * 3. + std::floor(u * 3.) * .5) - .5);
                return std::clamp(1. - std::min(HexX * 1.35 + HexY * .65, HexY * 1.5) * 2.2 + (.5 + .5 * std::sin(phase + random * TwoPi)) * .18, 0., 1.);
            }
            case 7:
            {
                const double Spiral = .5 + .5 * std::sin(Angle * 5. + std::log(Radius + .035) * 10. - phase);
                const double Rings = .5 + .5 * std::cos(Radius * TwoPi * 12. + phase * .6);
                return std::clamp(Spiral * .7 + Rings * .3, 0., 1.);
            }
            case 8:
            {
                const double Voronoi = std::min(std::hypot(u - random, v - Fraction(random * 7.31)), std::hypot(u - Fraction(random * 3.17), v - random));
                return std::clamp(1. - Voronoi * 2.4 + .15 * std::sin(phase + random * TwoPi), 0., 1.);
            }
            case 9:
            {
                const double BrickU = Fraction(u * 4. + (double) (y & 1) * .5);
                const double BrickV = Fraction(v * 6.);
                const double Mortar = std::min(std::min(BrickU, 1. - BrickU), std::min(BrickV, 1. - BrickV));
                return std::clamp(Mortar * 8. + random * .16, 0., 1.);
            }
            case 10:
            {
                const double Wave = std::sin((u + std::sin(v * TwoPi * 2. + phase) * .12) * TwoPi * 7.);
                const double Echo = std::cos((v + std::sin(u * TwoPi * 3. - phase) * .08) * TwoPi * 5.);
                return std::clamp(.5 + Wave * .32 + Echo * .18, 0., 1.);
            }
            default:
            {
                const double Star = .5 + .5 * std::sin(Angle * (8. + (double) (hash & 3)) + Radius * TwoPi * 9. - phase);
                const double Halo = std::exp(-Radius * (4. + random * 3.));
                return std::clamp(Star * .64 + Halo * .55, 0., 1.);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    D2D1_COLOR_F effect_t::SamplePalette(size_t palette, double amount) const noexcept
    {
        amount = std::clamp(amount, 0., 1.);
        switch (palette)
        {
            case 0: return HSVToColor((float)Fraction(.56 + amount * .22), .92f, (float)amount);
            case 1: return HSVToColor((float)Fraction(.96 + amount * .13), .88f, (float)std::pow(amount, .82));
            case 2: return HSVToColor((float)Fraction(.11 + amount * .08), .95f, (float)amount);
            case 3: return HSVToColor((float)Fraction(.34 + amount * .19), .82f, (float)std::pow(amount, .9));
            case 4: return HSVToColor((float)Fraction(.74 + amount * .18), .86f, (float)amount);
            case 5: return D2D1::ColorF((float)amount, (float)(amount * .78 + .05), (float)(amount * .32 + .02));
            case 6: return D2D1::ColorF((float)(amount * .22), (float)(amount * .9), (float)(amount * .78));
            default: return D2D1::ColorF((float)std::pow(amount, .75), (float)std::pow(amount, 1.35), (float)std::pow(amount, 2.1));
        }
    }

    /// <summary>
    /// 
    /// </summary>
    uint32_t effect_t::SampleTexture(double u, double v, double phase, int64_t detailLevel, double zoomFraction) const noexcept
    {
        const double ContinuousDepth = (double)detailLevel + zoomFraction;
        double Pattern = 0.;
        double Weight = 0.;
        for (int RelativeOctave = -3; RelativeOctave <= 8; ++RelativeOctave)
        {
            const int64_t GlobalOctave = detailLevel + RelativeOctave;
            const double VisualOctave = (double)GlobalOctave - ContinuousDepth;
            const double Envelope = SmoothStep(-2., -.25, VisualOctave) * (1. - SmoothStep(5.25, 7., VisualOctave));
            if (Envelope <= 1.e-6) continue;
            const double OctaveWeight = Envelope * std::exp2(-.72 * std::max(0., VisualOctave));
            const double Scale = 8. * std::exp2(VisualOctave);
            const double su = u * Scale, sv = v * Scale, fu = Fraction(su), fv = Fraction(sv);
            const int64_t cx = (int64_t)std::floor(su), cy = (int64_t)std::floor(sv);
            uint64_t Hash = (uint64_t)(cx * 0x632be59bd9b4e019LL) ^ (uint64_t)(cy * 0x9e3779b97f4a7c15LL);
            Hash ^= (uint64_t)GlobalOctave * 0x94d049bb133111ebULL; Hash ^= Hash >> 30; Hash *= 0xbf58476d1ce4e5b9ULL; Hash ^= Hash >> 27;
            const double Random = (double)(Hash & 0xffffu) / 65535.;
            const double A = SamplePattern(_RenderTextureA, fu, fv, phase, Random, cx, cy, Hash);
            const double B = SamplePattern(_RenderTextureB, fu, fv, phase, Random, cx, cy, Hash);
            Pattern += Lerp(A, B, _RenderTextureBlend) * OctaveWeight; Weight += OctaveWeight;
        }
        Pattern = std::clamp(Pattern / std::max(Weight, 1.e-6), 0., 1.);
        const double PaletteAmount = std::clamp(Pattern * (.86 + _TrebleEnergy * .14), 0., 1.);
        const D2D1_COLOR_F A = SamplePalette(_RenderPaletteA, PaletteAmount);
        const D2D1_COLOR_F B = SamplePalette(_RenderPaletteB, PaletteAmount);
        const double BrightnessFactor = (_Brightness == Brightness::Low) ? .72 : (_Brightness == Brightness::High) ? 1.18 : .94;
        const uint32_t r = (uint32_t)std::clamp(Lerp((double)A.r, (double)B.r, _RenderPaletteBlend) * BrightnessFactor * 255., 0., 255.);
        const uint32_t g = (uint32_t)std::clamp(Lerp((double)A.g, (double)B.g, _RenderPaletteBlend) * BrightnessFactor * 255., 0., 255.);
        const uint32_t b = (uint32_t)std::clamp(Lerp((double)A.b, (double)B.b, _RenderPaletteBlend) * BrightnessFactor * 255., 0., 255.);
        return 0xff000000u | (r << 16) | (g << 8) | b;
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
        const UINT32 Width = std::max<UINT32>(64, (UINT32)std::ceil(_Width * Resolution));
        const UINT32 Height = std::max<UINT32>(64, (UINT32)std::ceil(_Height * Resolution));
        if (_RenderTarget == renderTarget && _Bitmap && Width == _PixelWidth && Height == _PixelHeight)
            return S_OK;
        DeleteDeviceDependentResources();
        hr = __super::CreateDeviceDependentResources(renderTarget);
        if (FAILED(hr))
            return hr;
        _PixelWidth = Width;
        _PixelHeight = Height;
        _Pixels.assign((size_t)Width * Height, 0xff000000u);
        const auto Properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.f, 96.f);
        hr = renderTarget->CreateBitmap(D2D1::SizeU(Width, Height), _Pixels.data(), Width * sizeof(uint32_t), Properties, _Bitmap.ReleaseAndGetAddressOf());
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
        _Bitmap.Reset();
        _Pixels.clear();
        _PixelWidth = 0;
        _PixelHeight = 0;
        __super::DeleteDeviceDependentResources();
    }
}

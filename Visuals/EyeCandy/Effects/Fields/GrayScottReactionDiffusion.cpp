
/** $VER: GrayScottReactionDiffusion.cpp (2026.11.04) */

#include <pch.h>

#include "GrayScottReactionDiffusion.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>

namespace GrayScottReactionDiffusion
{
    using namespace Field;

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
        return std::format(L"Gray-Scott Reaction-Diffusion, {}, {} quality, {} speed", _Theme.Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Reset() noexcept
    {
        // Keep the numerical simulation independent of the physical back-buffer size.
        constexpr uint32_t MaximumLongSide[] = { 384, 640, 960 };

        const uint32_t LongSide  = MaximumLongSide[std::min<size_t>((size_t) _Quality, _countof(MaximumLongSide) - 1)];
        const double AspectRatio = (double) std::max(_Width, 1.f) / (double) std::max(_Height, 1.f);

        if (AspectRatio >= 1.)
        {
            _RasterWidth  = LongSide;
            _RasterHeight = std::max(64u, (uint32_t) std::lround((double) LongSide / AspectRatio));
        }
        else
        {
            _RasterHeight = LongSide;
            _RasterWidth  = std::max(64u, (uint32_t) std::lround((double) LongSide * AspectRatio));
        }

        const size_t Count = (size_t) _RasterWidth * _RasterHeight;

        _Pixels.assign(Count, PackRGB(0., 0., 0.));

        _U.assign(Count, 1.f);
        _V.assign(Count, 0.f);

        _NextU.resize(Count);
        _NextV.resize(Count);

        SeedPattern();

        _BackBuffer.Reset();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        constexpr double DefaultFrameTime = 1. / 60.;
        constexpr double MaximumFrameTime = .1;

        if (!std::isfinite(deltaTime) || deltaTime <= 0. || deltaTime > MaximumFrameTime)
            deltaTime = DefaultFrameTime;

        _Processor.Process(chunk, deltaTime);

        const double SpeedFactor = (_Speed == EyeCandy::Speed::Slow) ? .55 : (_Speed == EyeCandy::Speed::Fast) ? 1.45 : .90;

        _AnimationTime += deltaTime * SpeedFactor;

        _Interaction.Impulse *= std::exp(-2.8 * deltaTime);

        if (_AnimationTime > _NextTransitionTime)
        {
            _NextThemeIndex = (_NextThemeIndex + 1) % Field::theme_t::BuiltInCount;
            _NextTransitionTime = _AnimationTime + TransitionInterval;
        }

        if (_ThemeIndex != _NextThemeIndex)
        {
            _FromTheme = _Theme;
            _ToTheme = Field::theme_t::BuiltIn(_NextThemeIndex);
            _ThemeIndex = _NextThemeIndex;
            _TransitionTime = 0.;
        }

        _TransitionTime = std::min(_TransitionTime + deltaTime / .85, 1.);

        if (_TransitionTime < 1.)
            _Theme = Field::theme_t::Blend(_FromTheme, _ToTheme, _TransitionTime);

        Update();

        _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _RasterWidth * BytesPerPixel);

        renderTarget->Clear(Black);
        renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::OnMouseWheel(short wheelDelta) noexcept
    {
        if (wheelDelta == 0)
            return;

        _Interaction.Zoom = std::clamp(_Interaction.Zoom * std::pow(1.12, (float) wheelDelta / WHEEL_DELTA), .55, 2.2);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::Update()
    {
        if (_U.empty() || _Workers.empty())
            return;

        // Several well-known Gray-Scott regimes lie in this narrow feed/kill region. Audio moves smoothly through spots, mitosis and coral-like structures.
        const double Feed = std::clamp(.0335 + .0075 * _Processor._MidEnergy + .0020 * std::sin(_AnimationTime * .13), .028, .046);
        const double Kill = std::clamp(.0610 + .0060 * _Processor._TrebleEnergy - .0025 * _Processor._BassEnergy, .055, .069);

        work_t Work
        {
            .Feed = Feed,
            .Kill = Kill,
            .DiffusionU = 1.,
            .DiffusionV = .50 + .08 * _Processor._StereoWidth,
            .TimeStep = .92
        };

        InjectAudio(_AnimationTime);

        const size_t BaseSteps = (_Quality == EyeCandy::Quality::Low) ? 3u : (_Quality == EyeCandy::Quality::High) ? 7u : 5u;
        const size_t Steps = BaseSteps + (size_t) std::lround(_Processor._Peak);

        for (size_t Step = 0; Step < Steps; ++Step)
        {
            DispatchStep(Work);

            _U.swap(_NextU);
            _V.swap(_NextV);
        }

        Colorize();
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SeedPattern() noexcept
    {
        if (_U.empty())
            return;

        const int CX = (int) _RasterWidth  / 2;
        const int CY = (int) _RasterHeight / 2;

        const int Radius = std::max(5, (int) std::min(_RasterWidth, _RasterHeight) / 18);

        for (int y = -Radius; y <= Radius; ++y)
        {
            for (int x = -Radius; x <= Radius; ++x)
            {
                const int XX = CX + x;
                const int YY = CY + y;

                if (XX < 0 || YY < 0 || XX >= (int) _RasterWidth || YY >= (int) _RasterHeight)
                    continue;

                if (x * x + y * y > Radius * Radius)
                    continue;

                const size_t I = Index((uint32_t) XX, (uint32_t) YY);
                const double Noise = .5 + .5 * std::sin((double) x * 12.9898 + (double) y * 78.233);

                _U[I] = (float) (.42 + Noise * .08);
                _V[I] = (float) (.88 + Noise * .10);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::InjectAudio(double time) noexcept
    {
        const double Transient = std::max(0., _Processor._Peak - _LastImpulse * .82);

        _LastImpulse = std::max(_Processor._Peak, _LastImpulse * .91);
        _InjectionPhase += .035 + (_Processor._StereoWidth * .06);

        if (Transient < .035 && _Interaction.Impulse < .02)
            return;

        const double Strength = std::clamp(Transient * 2.2 + _Interaction.Impulse * .45, .08, 1.);

        const double OrbitX = std::sin(_InjectionPhase * 1.7 + time * .11);
        const double OrbitY = std::cos(_InjectionPhase * 1.3 - time * .09);

        const int CX = (int) std::lround((_RasterWidth  - 1) * (.5 + OrbitX * .31));
        const int CY = (int) std::lround((_RasterHeight - 1) * (.5 + OrbitY * .31));

        const int Radius = std::max(2, (int) std::lround(std::min(_RasterWidth, _RasterHeight) * (.010 + Strength * .018)));

        for (int y = -Radius; y <= Radius; ++y)
        {
            for (int x = -Radius; x <= Radius; ++x)
            {
                const int XX = (CX + x + (int) _RasterWidth)  % (int) _RasterWidth;
                const int YY = (CY + y + (int) _RasterHeight) % (int) _RasterHeight;

                const double D2 = (double) (x * x + y * y) / (double) (Radius * Radius);

                if (D2 > 1.)
                    continue;

                const float Falloff = (float) ((1. - D2) * Strength);

                const size_t I = Index((uint32_t) XX, (uint32_t) YY);

                _U[I] = std::max(0.f, _U[I] - Falloff * .45f);
                _V[I] = std::min(1.f, _V[I] + Falloff * .72f);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DispatchStep(const work_t & work)
    {
        {
            const std::lock_guard Lock(_WorkerMutex);

            _Work = work;

            _NextRow.store(0, std::memory_order_relaxed);

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
                _Workers.emplace_back(&effect_t::Worker, this);
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
    void effect_t::Worker() noexcept
    {
        size_t Generation = 0;

        for (;;)
        {
            work_t Work;

            {
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
                });

                if (_IsStopping)
                    return;

                Work       = _Work;
                Generation = _Generation;
            }

            // Amortize the atomic operation over several rows to reduce contention.
            constexpr uint32_t RowsPerJob = 8;

            for (;;)
            {
                const uint32_t FirstRow = _NextRow.fetch_add(RowsPerJob, std::memory_order_relaxed);

                if (FirstRow >= _RasterHeight)
                    break;

                GenerateRows(Work, FirstRow, FirstRow + RowsPerJob);
            }

            const std::lock_guard Lock(_WorkerMutex);

            if (--_PendingWorkers == 0)
                _CompletionCondition.notify_one();
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept
    {
        lastRow = std::min(lastRow, _RasterHeight);

        const uint32_t Width  = _RasterWidth;
        const uint32_t Height = _RasterHeight;

        const float Feed     = (float) work.Feed;
        const float KillFeed = (float) (work.Kill + work.Feed);

        const float DiffusionU = (float) work.DiffusionU;
        const float DiffusionV = (float) work.DiffusionV;

        const float TimeStep = (float) work.TimeStep;

        const float * const U = _U.data();
        const float * const V = _V.data();

        float * const NextU = _NextU.data();
        float * const NextV = _NextV.data();

        for (uint32_t Y = firstRow; Y < lastRow; ++Y)
        {
            const uint32_t YM = (Y == 0) ? Height - 1 : Y - 1;
            const uint32_t YP = (Y + 1 == Height) ? 0 : Y + 1;

            const size_t Row = (size_t) Y * Width;

            const size_t RowM = (size_t) YM * Width;
            const size_t RowP = (size_t) YP * Width;

            for (uint32_t X = 0; X < Width; ++X)
            {
                const uint32_t XM = (X == 0) ? Width - 1 : X - 1;
                const uint32_t XP = (X + 1 == Width) ? 0 : X + 1;

                const size_t I = Row + X;

                const float LapU = -U[I] + .2f * (U[Row + XM] + U[Row + XP] + U[RowM + X] + U[RowP + X]) + .05f * (U[RowM + XM] + U[RowM + XP] + U[RowP + XM] + U[RowP + XP]);
                const float LapV = -V[I] + .2f * (V[Row + XM] + V[Row + XP] + V[RowM + X] + V[RowP + X]) + .05f * (V[RowM + XM] + V[RowM + XP] + V[RowP + XM] + V[RowP + XP]);

                const float CurrentU = U[I];
                const float CurrentV = V[I];

                const float Reaction = CurrentU * CurrentV * CurrentV;

                NextU[I] = std::clamp(CurrentU + (DiffusionU * LapU - Reaction + Feed * (1.f - CurrentU)) * TimeStep, 0.f, 1.f);
                NextV[I] = std::clamp(CurrentV + (DiffusionV * LapV + Reaction - KillFeed * CurrentV) * TimeStep, 0.f, 1.f);
            }
        }
    }

    /// <summary>
    /// Colorizes the complete raster.
    /// </summary>
    void effect_t::Colorize() noexcept
    {
        for (uint32_t Y = 0; Y < _RasterHeight; ++Y)
        {
            const uint32_t YP = (Y + 1 == _RasterHeight) ? 0 : Y + 1;

            for (uint32_t X = 0; X < _RasterWidth; ++X)
            {
                const uint32_t XP = (X + 1 == _RasterWidth) ? 0 : X + 1;
                const size_t I = Index(X, Y);
                const double Edge = std::abs(_V[Index(XP, Y)] - _V[I]) + std::abs(_V[Index(X, YP)] - _V[I]);

                _Pixels[I] = Colorize(_U[I], _V[I], Edge);
            }
        }
    }

    /// <summary>
    /// Generates a color for one pixel.
    /// </summary>
    uint32_t effect_t::Colorize(double u, double v, double edge) const noexcept
    {
        const double Concentration = Clamp(v * 1.45 - u * .18 + .12);
        const double PatternLight  = .18 + .82 * std::pow(Clamp(v * 1.75 + edge * 2.2), .62);
        const double AudioLight    = .36 + (.48 * std::sqrt(Clamp(_Processor._Loudness))) + (.16 * _Processor._Peak);

        const double H = _Theme.BaseHue + (_Theme.HueSpan * Concentration) + (_Theme.BassHue * _Processor._BassEnergy) + (_Theme.MidHue * _Processor._MidEnergy) + (_Theme.TrebleHue * _Processor._TrebleEnergy);
        const double S = Clamp(_Theme.Saturation + (.20 * edge) + (.12 * _Processor._TrebleEnergy));
        const double V = Clamp(_Theme.Brightness * PatternLight * AudioLight);

        double R, G, B;

        HSVToRGB(H, S, V, R, G, B);

        R = Clamp(R + _Theme.BassRed    * _Processor._BassEnergy);
        G = Clamp(G + _Theme.MidGreen   * _Processor._MidEnergy);
        B = Clamp(B + _Theme.TrebleBlue * _Processor._TrebleEnergy);

        const double BrightnessFactor = (_Brightness == EyeCandy::Brightness::Low) ? .50 : (_Brightness == EyeCandy::Brightness::High) ? 1.50 : 1.;

        return PackRGB(R * BrightnessFactor, G * BrightnessFactor, B * BrightnessFactor);
    }
}

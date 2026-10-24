
/** $VER: Aurora.cpp (2026.10.23) **/

#include <pch.h>

#include "Aurora.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace Aurora
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
        return std::format(L"Aurora, {}, {} quality, {} speed", _Theme.Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
    }

    /// <summary>
    ///
    /// </summary>
    void effect_t::Reset() noexcept
    {
        constexpr uint32_t Dividers[] = { 9, 6, 3 };

        const float Scale = std::max(_Width, _Height) / std::min(_Width, _Height);

        const float Divider = (float) Dividers[std::min<size_t>((size_t) _Quality, _countof(Dividers) - 1)];

        _RasterWidth  = std::max((uint32_t) std::lround(_Width  * Scale / Divider), 1u);
        _RasterHeight = std::max((uint32_t) std::lround(_Height * Scale / Divider), 1u);

        _Pixels.resize((size_t) _RasterWidth * _RasterHeight);

        // Make sure the Direct2D bitmap gets recreated.
        _BackBuffer.Reset();
    }

    /// <summary>
    /// Renders this instance.
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        // A track change can make the playback-derived time delta jump backwards or become very large. Negative deltas reverse exponential decay and make the
        // smoothed balance grow without bounds, moving every blob off-screen.
        constexpr double DefaultFrameTime = 1. / 60.;
        constexpr double MaximumFrameTime = 0.1;

        if (!std::isfinite(deltaTime) || deltaTime <= 0. || deltaTime > MaximumFrameTime)
            deltaTime = DefaultFrameTime;

        // Update the pixel matrix.
        {
            _Audio = _Analyzer.Analyze(chunk, deltaTime);

            {
                const double SpeedFactor = [&]() noexcept
                {
                    switch (_Speed)
                    {
                        case EyeCandy::Speed::Slow:
                            return 0.30;

                        default:
                        case EyeCandy::Speed::Normal:
                            return 0.45;

                        case EyeCandy::Speed::Fast:
                            return 0.85;
                    }
                }();

                _AnimationTime += deltaTime * SpeedFactor;
                _ThemeTime     += deltaTime;

                // Keep the trigonometric argument bounded during long-running sessions.
                constexpr double WrapTime = TwoPi * 1024.;

                if (_AnimationTime >= WrapTime)
                    _AnimationTime = std::fmod(_AnimationTime, TwoPi);
            }

            _Interaction.Impulse *= std::exp(-2.6 * deltaTime);

            if (_AnimationTime > _NextTransitionTime)
            {
                ++_NextThemeIndex;

                if (_NextThemeIndex == Field::theme_t::BuiltInCount)
                    _NextThemeIndex = 0;

                _NextTransitionTime = _AnimationTime + TransitionInterval;
            }

            if (_ThemeIndex != _NextThemeIndex)
            {
                _FromTheme = _Theme;
                _ToTheme   = Field::theme_t::BuiltIn(_NextThemeIndex);

                _ThemeIndex = _NextThemeIndex;
                _TransitionTime = 0.;
            }

            _TransitionTime = std::min(_TransitionTime + (deltaTime / .40), 1.);

            if (_TransitionTime < 1.)
                _Theme = Field::theme_t::Blend(_FromTheme, _ToTheme, _TransitionTime);

            Update(_AnimationTime);
        }

        // Update the Direct2D bitmap.
        {
            const auto Size = D2D1::SizeU(_RasterWidth, _RasterHeight); // Can be different than the render target size.

            if ((_BackBuffer == nullptr) || (_BackBuffer->GetPixelSize().width != Size.width) || (_BackBuffer->GetPixelSize().height != Size.height))
            {
                _BackBuffer.Reset();

                renderTarget->CreateBitmap(Size, nullptr, 0, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), _BackBuffer.GetAddressOf());
            }

            if (_BackBuffer)
                _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _RasterWidth * BytesPerPixel);
        }

        // Render the bitmap.
        {
            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _RasterWidth * BytesPerPixel);

            renderTarget->Clear(Black);

            const auto Size = renderTarget->GetSize();

            renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
        }
    }

    /// <summary>
    /// Builds the parameters for broad, vertically flowing aurora curtains. Bass controls the lower glow and curtain width,
    /// midrange bends the folds, and treble sharpens the luminous edges and vertical shimmer.
    /// </summary>
    void effect_t::Update(double time)
    {
        if (_Pixels.empty() || _Workers.empty())
            return;

        const double ClampedZoom = std::clamp(_Interaction.Zoom, .55, 2.2); // Prevent extreme or invalid scaling.
        const double LowEnergy   = Clamp((_Audio.Bass   * .82) + (_Audio.Loudness * .18));
        const double MidEnergy   = Clamp((_Audio.Mid    * .84) + (_Audio.Loudness * .16));
        const double HighEnergy  = Clamp((_Audio.Treble * .88) + (_Audio.Peak     * .12));
        const double BroadEnergy = Clamp((LowEnergy + MidEnergy + HighEnergy) / 3.);

        work_t Work
        {
            .Time          = time,
            .InverseWidth  = 1. / (double) _RasterWidth,
            .InverseHeight = 1. / (double) _RasterHeight,
            .CurtainScale  = (2.8 + (1.4 * LowEnergy)) / ClampedZoom,
            .Drift         = .10 + (.48 * BroadEnergy),
            .FoldStrength  = .10 + (.31 * MidEnergy) + (.08 * LowEnergy),
            .FoldFrequency = 2.3 + (4.7 * MidEnergy),
            .VerticalFlow  = .18 + (1.15 * HighEnergy),
            .LowEnergy     = LowEnergy,
            .MidEnergy     = MidEnergy,
            .HighEnergy    = HighEnergy,
            .BroadEnergy   = BroadEnergy,
            .BalanceOffset = .16 * _Audio.Balance,
            .Horizon       = .80 - (.05 * LowEnergy),
            .HasImpulse    = (_Interaction.Impulse != 0.)
        };

        {
            const std::lock_guard Lock(_WorkerMutex);

            _Work = Work;

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
    /// Converts curtain intensity, fold brightness, and height into an opaque 32-bit RGB color.
    /// Broad frequency bands shift hue and brightness while preserving the active theme.
    /// </summary>
    uint32_t effect_t::Colorize(double intensity, double fold, double height) const noexcept
    {
        const double Intensity = Clamp(intensity);
        const double Fold      = Clamp(fold);
        const double Height    = Clamp(height);

        const double H =
            _Theme.BaseHue +
           (_Theme.HueSpan   * ((Fold * .72) + (Height * .18))) +
           (_Theme.BassHue   * _Audio.Bass) +
           (_Theme.MidHue    * _Audio.Mid) +
           (_Theme.TrebleHue * _Audio.Treble);

        const double S = Clamp((_Theme.Saturation * .78) + (.14 * Fold) + (.08 * _Audio.Treble));

        const double Ambient = .012 + (.055 * std::pow(Clamp(_Audio.Loudness), .6));

        const double V = Clamp(Ambient + (_Theme.Brightness * Intensity * (.56 + (.44 * Fold))));

        double R, G, B;

        HSVToRGB(H, S, V, R, G, B);

        R = Clamp(R + (_Theme.BassRed    * _Audio.Bass)   + (.06 * Fold));
        G = Clamp(G + (_Theme.MidGreen   * _Audio.Mid)    + (.14 * Intensity));
        B = Clamp(B + (_Theme.TrebleBlue * _Audio.Treble) + (.10 * Fold));

        const double BrightnessFactor = [&]() noexcept
        {
            switch (_Brightness)
            {
                case EyeCandy::Brightness::Low:
                    return .50;

                default:
                case EyeCandy::Brightness::Medium:
                    return 1.00;

                case EyeCandy::Brightness::High:
                    return 1.50;
            }
        }();

        R *= BrightnessFactor;
        G *= BrightnessFactor;
        B *= BrightnessFactor;

        return PackRGB(R, G, B);
    }

    /// <summary>
    /// Creates one field-generation worker per logical CPU core.
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
    /// Signals all workers and waits for them to finish.
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
    /// Processes dynamically assigned rows for each generated frame.
    /// </summary>
    void effect_t::Worker() noexcept
    {
        size_t Generation = 0;

        for (;;)
        {
            work_t Field;
            {
                std::unique_lock Lock(_WorkerMutex);

                _WorkerCondition.wait(Lock, [this, &Generation]() noexcept
                {
                    return _IsStopping || (_Generation != Generation);
                });

                if (_IsStopping)
                    return;

                Field = _Work;

                Generation = _Generation;
            }

            for (;;)
            {
                const uint32_t Row = _NextRow.fetch_add(1, std::memory_order_relaxed);

                if (Row >= (uint32_t) _RasterHeight)
                    break;

                GenerateRows(Field, Row, Row + 1);
            }

            {
                const std::lock_guard Lock(_WorkerMutex);

                if (--_PendingWorkers == 0)
                    _CompletionCondition.notify_one();
            }
        }
    }

    /// <summary>
    /// Generates a contiguous range of luminous aurora-curtain rows.
    /// </summary>
    void effect_t::GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept
    {
        lastRow = std::min(lastRow, (uint32_t) _RasterHeight);

        for (uint32_t y = firstRow; y < lastRow; ++y)
        {
            const double V = (double) y * work.InverseHeight;
            const size_t PixelRowOffset = (size_t) (y * (uint32_t) _RasterWidth);

            auto * Row = &_Pixels[PixelRowOffset];

            for (uint32_t x = 0; x < (uint32_t) _RasterWidth; ++x, ++Row)
            {
                const double U = (double) x * work.InverseWidth + work.BalanceOffset;
                const double Horizontal = (U - .5) * work.CurtainScale;
                const double VerticalMask = Clamp((work.Horizon - V) * 7.5) * Clamp((V + .04) * 4.2);

                double Intensity = 0.;
                double BrightestFold = 0.;

                // Layer several broad curtains. Each layer has independent folds and vertical phase so the field remains organic.
                for (uint32_t Layer = 0; Layer < 4; ++Layer)
                {
                    const double LayerPhase = (double) Layer * 1.731;
                    const double FoldWave   = std::sin((Horizontal * work.FoldFrequency) + LayerPhase + (work.Time * work.Drift)) + .43 * std::sin((Horizontal * (work.FoldFrequency * 2.17)) - LayerPhase - (work.Time * .19));
                    const double Center     = .34 + ((double) Layer * .12) + (work.FoldStrength * FoldWave) + (.035 * std::sin(work.Time * .23 + LayerPhase));
                    const double Width      = .052 + ((double) Layer * .012) + (.055 * work.LowEnergy);
                    const double Distance   = std::abs(V - Center);
                    const double Curtain    = std::exp(-(Distance * Distance) / (Width * Width));

                    const double RayPhase       = (V * (34. + (42. * work.HighEnergy))) - (work.Time * work.VerticalFlow) + (Horizontal * 3.1) + LayerPhase;
                    const double Rays           = .54 + (.46 * std::sin(RayPhase));
                    const double Fold           = Clamp(.5 + (.5 * std::sin((Horizontal * 13.7) + FoldWave * 2.6 + LayerPhase)));
                    const double LayerIntensity = Curtain * (.32 + (.46 * work.BroadEnergy) + (.22 * Rays)) * (.68 + (.32 * Fold));

                    Intensity += LayerIntensity;

                    BrightestFold = std::max(BrightestFold, Fold * Curtain);
                }

                // Add fine vertical striations mostly from the high band, plus a soft low-frequency glow below the curtains.
                const double Striation = .5 + (.5 * std::sin((U * (86. + (48. * work.HighEnergy))) + work.Time * work.VerticalFlow));

                Intensity *= .76 + (.24 * Striation * work.HighEnergy);
                Intensity += .13 * work.LowEnergy * std::exp(-std::pow((V - .70) / .18, 2.));
                Intensity *= VerticalMask;

                // Keep the lower sky nearly black with only a subtle theme-colored reflection beneath the horizon.
                if (V >= work.Horizon)
                {
                    const double Reflection = Clamp(1. - ((V - work.Horizon) * 5.5));

                    Intensity      = .012 + (Intensity * .11 * Reflection);
                    BrightestFold *= .12 * Reflection;
                }

                *Row = Colorize(Intensity, BrightestFold, 1. - V);
            }
        }
    }
}

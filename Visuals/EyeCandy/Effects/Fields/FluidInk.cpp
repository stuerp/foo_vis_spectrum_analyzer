
/** $VER: FluidInk.cpp (2026.10.23) **/

#include <pch.h>

#include "FluidInk.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace FluidInk
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
        return std::format(L"Fluid Ink, {}, {} quality, {} speed", _Theme.Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
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

            _TransitionTime = std::min(_TransitionTime + (deltaTime / 0.85), 1.);

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
            renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

            const auto Size = renderTarget->GetSize();

            renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
        }
    }

    /// <summary>
    /// Builds a procedural fluid-flow field. Bass pushes broad streams across the surface, midrange adds curl,
    /// treble sharpens filament detail, and peaks inject short radial forces into the ink.
    /// </summary>
    void effect_t::Update(double time)
    {
        if (_Pixels.empty() || _Workers.empty())
            return;

        const double ClampedZoom = std::clamp(_Interaction.Zoom, .55, 2.2); // Prevent extreme or invalid scaling.
        const double Transient   = Clamp(std::max(0., _Audio.Peak - .42) * 1.75 + std::abs(_Interaction.Impulse));

        work_t Work
        {
            .Time            = time,
            .InverseWidth    = 1. / (double) _RasterWidth,
            .InverseHeight   = 1. / (double) _RasterHeight,
            .CoordinateScale = (1.12 + (.20 * _Audio.Bass)) / ClampedZoom,
            .HorizontalScale = (double) _RasterWidth / std::max(1., (double) _RasterHeight),
            .FlowSpeed       = .18 + (.72 * _Audio.Loudness),
            .BassPush        = .18 + (1.75 * _Audio.Bass),
            .MidCurl         = .28 + (1.45 * _Audio.Mid),
            .TrebleDetail    = .12 + (1.35 * _Audio.Treble),
            .TransientForce  = Transient,
            .ForcePositionX  = .48 * std::sin(time * .31 + _Audio.Balance * 2.1),
            .ForcePositionY  = .34 * std::cos(time * .23 - _Audio.Balance * 1.4),
            .ColorPhase      = time * (.09 + (.22 * _Audio.Treble)),
            .BalanceOffset   = .24 * _Audio.Balance,
            .HasImpulse      = (_Interaction.Impulse != 0.)
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
    /// Converts the ink density, stream identity, and filament highlight to an opaque 32-bit RGB color.
    /// Adjacent streams receive separated hues while dense folds retain dark, ink-like cores.
    /// </summary>
    uint32_t effect_t::Colorize(double density, double stream, double highlight) const noexcept
    {
        const double Density   = Clamp(density);
        const double Stream    = Clamp(stream);
        const double Highlight = Clamp(highlight);

        const double Hue = _Theme.BaseHue + (_Theme.HueSpan * (.12 + (.88 * Stream))) + (_Theme.BassHue * _Audio.Bass) + (_Theme.MidHue * _Audio.Mid) + (_Theme.TrebleHue * _Audio.Treble);

        const double Saturation = Clamp((_Theme.Saturation * (.74 + (.24 * Density))) + (.12 * Stream));
        const double Ambient    = .035 + (.22 * std::pow(Clamp(_Audio.Loudness), .55));
        const double Brightness = Clamp(Ambient + (_Theme.Brightness * (.58 * Density + .34 * Highlight)));

        double Red, Green, Blue;
        HSVToRGB(Hue, Saturation, Brightness, Red, Green, Blue);

        Red   = Clamp(Red   + (_Theme.BassRed    * _Audio.Bass)   + (.10 * Highlight));
        Green = Clamp(Green + (_Theme.MidGreen   * _Audio.Mid)    + (.07 * Highlight));
        Blue  = Clamp(Blue  + (_Theme.TrebleBlue * _Audio.Treble) + (.14 * Highlight));

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

        Red   *= BrightnessFactor;
        Green *= BrightnessFactor;
        Blue  *= BrightnessFactor;

        const uint32_t R = (uint32_t) ToRGB(Red);
        const uint32_t G = (uint32_t) ToRGB(Green);
        const uint32_t B = (uint32_t) ToRGB(Blue);

        return 0xFF000000u | (R << 16u) | (G << 8u) | B;
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
    /// Generates a contiguous range of fluid-ink rows.
    /// </summary>
    void effect_t::GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept
    {
        lastRow = std::min(lastRow, (uint32_t) _RasterHeight);

        for (uint32_t y = firstRow; y < lastRow; ++y)
        {
            const double InkY = (2. * (double) y * work.InverseHeight - 1.) * work.CoordinateScale - _Interaction.Y;
            const size_t PixelRowOffset = (size_t) (y * (uint32_t) _RasterWidth);

            for (uint32_t x = 0; x < (uint32_t) _RasterWidth; ++x)
            {
                double InkX = (2. * (double) x * work.InverseWidth - 1.) * work.HorizontalScale * work.CoordinateScale - _Interaction.X + work.BalanceOffset;
                double FlowY = InkY;

                // Bass supplies the broad lateral pressure that pushes the colored streams through the field.
                InkX  -= work.Time * work.FlowSpeed * (.30 + work.BassPush);
                FlowY += .16 * work.BassPush * std::sin(InkX * 1.35 - work.Time * .42);

                // Iterated curl warps coordinates without requiring a persistent fluid simulation buffer.
                for (size_t Iteration = 0; Iteration < 3; ++Iteration)
                {
                    const double Scale    = 1.55 + ((double) Iteration * 1.37);
                    const double Phase    = work.Time * (.17 + ((double) Iteration * .08));
                    const double CurlX    = std::sin(FlowY * Scale + Phase) + std::cos((InkX + FlowY) * Scale * .63 - Phase);
                    const double CurlY    = std::cos(InkX * Scale - Phase) - std::sin((InkX - FlowY) * Scale * .71 + Phase);
                    const double Strength = (.11 + (.055 * (double) Iteration)) * work.MidCurl;

                    InkX  += CurlX * Strength;
                    FlowY += CurlY * Strength;
                }

                // A transient creates a compact radial shove followed by a curling wake.
                const double ForceX               = InkX - work.ForcePositionX;
                const double ForceY               = FlowY - work.ForcePositionY;
                const double ForceDistanceSquared = ForceX * ForceX + ForceY * ForceY;
                const double ForceFalloff         = work.TransientForce * std::exp(-ForceDistanceSquared * 3.8);
                const double InverseDistance      = 1. / std::sqrt(ForceDistanceSquared + .012);

                InkX  += (ForceX * InverseDistance + ForceY * .72) * ForceFalloff * .48;
                FlowY += (ForceY * InverseDistance - ForceX * .72) * ForceFalloff * .48;

                if (work.HasImpulse)
                {
                    InkX  += ForceX * _Interaction.Impulse * .035 * InverseDistance;
                    FlowY += ForceY * _Interaction.Impulse * .035 * InverseDistance;
                }

                const double RibbonCoordinate = InkX * (2.25 + (.45 * work.BassPush)) + .82 * std::sin(FlowY * 3.15 + work.Time * .27) + .34 * work.TrebleDetail * std::sin((InkX - FlowY) * 8.7 - work.Time * .56);

                const double StreamWave = std::sin(RibbonCoordinate);
                const double FineWave = std::sin(RibbonCoordinate * 2.03 + FlowY * 5.4 + work.ColorPhase);
                const double Ridge = std::pow(1. - std::abs(StreamWave), 3.4);
                const double Filament = std::pow(1. - std::abs(FineWave), 8.);
                const double Density = Clamp(.12 + (.68 * Ridge) + (.34 * Filament) + (.26 * ForceFalloff));
                const double Stream = .5 + (.5 * std::sin(RibbonCoordinate * .47 + work.ColorPhase));
                const double Highlight = Clamp((.62 * Filament) + (.38 * Ridge * work.TrebleDetail) + (.44 * ForceFalloff));

                _Pixels[PixelRowOffset + x] = Colorize(Density, Stream, Highlight);
            }
        }
    }
}


/** $VER: Metaballs.cpp (2026.10.23) **/

#include <pch.h>

#include "Metaballs.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace Metaballs
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
        return std::format(L"Metaballs, {}, {} quality, {} speed", _Theme.Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
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

            if (_ThemeTime > _NextTransitionTime)
            {
                ++_NextThemeIndex;

                if (_NextThemeIndex == Field::theme_t::BuiltInCount)
                    _NextThemeIndex = 0;

                _NextTransitionTime = _ThemeTime + TransitionInterval;
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

        // Render the bitmap.
        {
            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _RasterWidth * BytesPerPixel);

            renderTarget->Clear(Black);

            const auto Size = renderTarget->GetSize();

            renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
        }
    }

    /// <summary>
    /// Positions a small set of circular field sources. The rendered shape is not a collection of drawn circles:
    /// every source contributes to one shared scalar field, whose iso-surface naturally joins and separates.
    /// </summary>
    void effect_t::Update(double time)
    {
        if (_Pixels.empty() || _Workers.empty())
            return;

        const double Zoom     = std::clamp(_Interaction.Zoom, .55, 2.2);
        const double Bass     = Clamp(_Audio.Bass);
        const double Mid      = Clamp(_Audio.Mid);
        const double Treble   = Clamp(_Audio.Treble);
        const double Loudness = Clamp(_Audio.Loudness);

        work_t Work {};

        Work.InverseWidth   = 1. / (double) _RasterWidth;
        Work.InverseHeight  = 1. / (double) _RasterHeight;
        Work.AspectRatio    = (double) _RasterWidth / (double) _RasterHeight;
        Work.Threshold      = 1.;
        Work.Softness       = .028 + (.018 * Loudness);
        Work.EdgeWidth      = .045 + (.020 * Treble);
        Work.BackgroundGlow = .004 + (.018 * Loudness);

        // Three large bass bodies, two mid bodies and two small treble satellites keep the source geometry readable.
        constexpr double BaseX[BlobCount] = { .32, .50, .68, .40, .60, .45, .55 };
        constexpr double BaseY[BlobCount] = { .48, .38, .52, .64, .65, .25, .76 };
        constexpr double BaseRadius[BlobCount] = { .145, .165, .140, .105, .105, .070, .062 };

        for (size_t i = 0; i < BlobCount; ++i)
        {
            const double Index     = (double) i;
            const double Band      = (i < 3) ? Bass : ((i < 5) ? Mid : Treble);
            const double Direction = ((i & 1) == 0) ? 1. : -1.;
            const double Speed     = (i < 3) ? (.46 + .34 * Bass) : ((i < 5) ? (.72 + .58 * Mid) : (1.05 + 1.10 * Treble));
            const double Phase     = time * Speed * Direction + Index * 1.713;
            const double Travel    = ((i < 3) ? .105 : ((i < 5) ? .145 : .190)) * (.65 + .35 * Band);

            Work.BlobX[i] = BaseX[i] + _Interaction.X * .18 + _Audio.Balance * .08 + std::cos(Phase) * Travel;
            Work.BlobY[i] = BaseY[i] + _Interaction.Y * .18 + std::sin(Phase * (1.13 + .03 * Index)) * Travel * .72;

            const double Expansion = (i < 3) ? (.060 * Bass) : ((i < 5) ? (.042 * Mid) : (.025 * Treble));

            Work.BlobRadius[i] = (BaseRadius[i] + Expansion) / Zoom;
            Work.BlobWeight[i] = .90 + (.28 * Band) + ((i == 1) ? .18 * Loudness : 0.);
        }

        // A user impulse briefly pulls the field sources together, producing an obvious merge before they separate again.
        if (_Interaction.Impulse > 0.)
        {
            const double Pull = .36 * Clamp(_Interaction.Impulse);

            for (size_t i = 0; i < BlobCount; ++i)
            {
                Work.BlobX[i] = std::lerp(Work.BlobX[i], .5 + _Interaction.X * .18, Pull);
                Work.BlobY[i] = std::lerp(Work.BlobY[i], .5 + _Interaction.Y * .18, Pull);
                Work.BlobRadius[i] *= 1. + (.10 * Pull);
            }
        }

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
    /// Colors the iso-surface, its narrow contour and the field inside it.
    /// Pixels below the iso-value remain dark so the merging silhouettes stay unmistakable.
    /// </summary>
    uint32_t effect_t::Colorize(double surface, double interior, double edge, double glow) const noexcept
    {
        surface  = Clamp(surface);
        interior = Clamp(interior);
        edge     = Clamp(edge);
        glow     = Clamp(glow);

        const double H = _Theme.BaseHue + (_Theme.HueSpan * (.12 + .72 * interior)) + (_Theme.BassHue * _Audio.Bass) + (_Theme.MidHue * _Audio.Mid) + (_Theme.TrebleHue * _Audio.Treble);
        const double S = Clamp(_Theme.Saturation + .08 - .22 * edge);
        const double V = Clamp(glow + surface * (_Theme.Brightness * (.36 + .50 * interior)) + .28 * edge);

        double R, G, B;

        HSVToRGB(H, S, V, R, G, B);

        R = Clamp(R + surface * _Theme.BassRed    * _Audio.Bass   + .12 * edge);
        G = Clamp(G + surface * _Theme.MidGreen   * _Audio.Mid    + .15 * edge);
        B = Clamp(B + surface * _Theme.TrebleBlue * _Audio.Treble + .20 * edge);

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
    /// Generates a contiguous range of rows from a classic summed inverse-square metaball field.
    /// The iso-value at Field == Threshold is the visible boundary; overlapping sources therefore merge mathematically.
    /// </summary>
    void effect_t::GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept
    {
        lastRow = std::min(lastRow, (uint32_t) _RasterHeight);

        for (uint32_t y = firstRow; y < lastRow; ++y)
        {
            const double V = ((double) y + .5) * work.InverseHeight;
            const size_t PixelRowOffset = (size_t) y * (size_t) _RasterWidth;

            auto * Row = &_Pixels[PixelRowOffset];

            for (uint32_t x = 0; x < (uint32_t) _RasterWidth; ++x, ++Row)
            {
                const double U = ((double) x + .5) * work.InverseWidth;

                double Field = 0.;

                for (size_t i = 0; i < BlobCount; ++i)
                {
                    const double DX = (U - work.BlobX[i]) * work.AspectRatio;
                    const double DY =  V - work.BlobY[i];

                    const double DistanceSquared = DX * DX + DY * DY + 1e-8;
                    const double RadiusSquared   = work.BlobRadius[i] * work.BlobRadius[i];

                    Field += work.BlobWeight[i] * RadiusSquared / DistanceSquared;
                }

                // Smooth only a narrow band around the iso-surface. This preserves round bodies and visible necks.
                const double Lower = work.Threshold - work.Softness;
                const double Upper = work.Threshold + work.Softness;

                double Surface = Clamp((Field - Lower) / (Upper - Lower));

                Surface = Surface * Surface * (3. - 2. * Surface);

                const double Interior = Clamp((Field - work.Threshold) / 2.25);
                const double Edge     = Surface * Clamp(1. - std::abs(Field - work.Threshold) / work.EdgeWidth);
                const double Glow     = work.BackgroundGlow * Clamp(Field / work.Threshold) * (1. - Surface);

                *Row = Colorize(Surface, Interior, Edge, Glow);
            }
        }
    }
}

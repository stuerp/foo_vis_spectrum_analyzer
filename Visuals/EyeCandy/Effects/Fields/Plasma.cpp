
/** $VER: Plasma.cpp (2026.10.23) **/

#include <pch.h>

#include "Plasma.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace Plasma
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
        return std::format(L"Plasma, {}, {} quality, {} speed", _Theme.Name, Qualities[(size_t) _Quality], Speeds[(size_t) _Speed]);
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
            _Audio = _Analyzer.Analyze(chunk, deltaTime);// : (_Analyzer.Decay(DeltaTime), _Analyzer.GetAudioFeatures());

            {
                const double SpeedFactor = [&]() noexcept
                {
                    switch (_Speed)
                    {
                        case EyeCandy::Speed::Slow:
                            return 0.45;

                        default:
                        case EyeCandy::Speed::Normal:
                            return 0.75;

                        case EyeCandy::Speed::Fast:
                            return 1.15;
                    }
                }();

                _AnimationTime += deltaTime * SpeedFactor;
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

        // Render the bitmap.
        {
            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _RasterWidth * BytesPerPixel);

            renderTarget->Clear(Black);

            const auto Size = renderTarget->GetSize();

            renderTarget->DrawBitmap(_BackBuffer.Get(), _Rectangle);
        }
    }

    /// <summary>
    /// Generates the plasma by mapping each pixel into a centered coordinate system, combining several animated sine-wave fields,
    /// normalizing the result, and converting it to a theme-dependent color.
    /// The audio controls scale, movement, spatial distortion, and animation speed.
    /// </summary>
    void effect_t::Update(double time)
    {
        if (_Pixels.empty() || _Workers.empty())
            return;

        const double ClampedZoom = std::clamp(_Interaction.Zoom, .55, 2.2); // Prevent extreme or invalid scaling.
        const double CoordinateScale = (2.3 - (.55 * _Audio.Bass)) / ClampedZoom;

        work_t Work
        {
            .Time                 = time,
            .InverseWidth         = 1. / (double) _RasterWidth,
            .InverseHeight        = 1. / (double) _RasterHeight,
            .CoordinateScale      = CoordinateScale,
            .HorizontalScale      = CoordinateScale * ((double) _RasterWidth / (double) _RasterHeight),
            .HorizontalFrequency  = 2.2 + 1.8 * _Audio.Mid,
            .DiagonalFrequency    = 2.7 + 1.2 * _Audio.Bass,
            .RadialFrequency      = 7.  + 3.  * _Audio.Bass,
            .HorizontalPhase      = time * (.70 + .7 * _Audio.Treble),
            .DiagonalPhase        = time * (.55 + _Audio.Mid),
            .RadialPhase          = time * (1.1 + _Audio.Peak),
            .DetailPhase          = time * .42,
            .RipplePhase          = time * 9.,
            .ColorPhase           = time * .12 + _Audio.Treble * 2.,
            .BalanceOffset        = .28 * _Audio.Balance,
            .RadialCenterX        = .65 * std::sin(time * .31),
            .RadialCenterY        = .55 * std::cos(time * .27),
            .HasImpulse           = (_Interaction.Impulse != 0.)
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
    /// Converts the normalized plasma field into an opaque 32-bit RGB color.
    /// It derives a base color in HSV space, lets each audio band influence the palette, converts the result to RGB, applies final per-channel accents, and packs the channels as 0xAARRGGBB.
    /// </summary>
    uint32_t effect_t::Colorize(double value) const noexcept
    {
        struct colorization_t
        {
            // Audio brightness: Loudness and transient response.
            double MinimumBrightness       = .20;
            double LoudnessBrightness      = .68;
            double PeakBrightness          = .16;
            double TrebleSaturation        = .16;

            // Spatial brightness shaping across the plasma intensity range.
            double IntensityBrightnessBase = .62;
            double IntensityBrightnessSpan = .38;
            double IntensityCurveFrequency = Pi;
            double IntensityCurvePhase     = 0.;

            // Controls how loudness is compressed or expanded:
            //   < 1.0 emphasizes quiet audio
            //   = 1.0 gives a linear response
            //   > 1.0 emphasizes loud audio
            double LoudnessExponent        = .50;

            //TrebleSaturation = 0.;   // No audio-driven saturation
            //TrebleSaturation = .10;  // Subtle
            //TrebleSaturation = .16;  // Original behavior
            //TrebleSaturation = .30;  // Strong, vivid response

            //LoudnessExponent = .35; // Very responsive to quiet audio
            //LoudnessExponent = .50; // Original square-root response
            //LoudnessExponent = .75; // Moderately compressed response
            //LoudnessExponent = 1.00; // Linear response
            //LoudnessExponent = 1.50; // Mostly responds to loud passages
        } Colorization;

        colorization_t SoftColorization
        {
            .MinimumBrightness       = .28,
            .LoudnessBrightness      = .48,
            .PeakBrightness          = .08,
            .TrebleSaturation        = .08,
            .IntensityBrightnessBase = .78,
            .IntensityBrightnessSpan = .22,
            .IntensityCurveFrequency = Pi,
            .IntensityCurvePhase     = 0.,
            .LoudnessExponent        = .45
        };
        
        colorization_t PunchyColorization
        {
            .MinimumBrightness       = .10,
            .LoudnessBrightness      = .78,
            .PeakBrightness          = .30,
            .TrebleSaturation        = .25,
            .IntensityBrightnessBase = .42,
            .IntensityBrightnessSpan = .58,
            .IntensityCurveFrequency = Pi,
            .IntensityCurvePhase     = 0.,
            .LoudnessExponent        = .70
        };

        colorization_t PsychedelicColorization
        {
            .MinimumBrightness       = .18,
            .LoudnessBrightness      = .65,
            .PeakBrightness          = .22,
            .TrebleSaturation        = .32,
            .IntensityBrightnessBase = .55,
            .IntensityBrightnessSpan = .45,
            .IntensityCurveFrequency = 3. * Pi,
            .IntensityCurvePhase     = Pi * .25,
            .LoudnessExponent        = .40
        };

        const double PlasmaIntensity = Clamp(value);

        // Shape the loudness response independently from the analyzer. The default exponent of .5 reproduces the original square-root response.
        const double LoudnessResponse = std::pow(Clamp(_Audio.Loudness), Colorization.LoudnessExponent);

        // Map plasma intensity across the theme palette, then shift its hue independently according to the energy in each frequency band.
        const double H =
            _Theme.BaseHue +
            _Theme.HueSpan   * PlasmaIntensity +
            _Theme.BassHue   * _Audio.Bass +
            _Theme.MidHue    * _Audio.Mid +
            _Theme.TrebleHue * _Audio.Treble;

        // Treble can increase saturation to make high-frequency activity appear sharper and more colorful.
        const double S = Clamp(_Theme.Saturation + (Colorization.TrebleSaturation * _Audio.Treble));

        // Shape brightness across the plasma field. With the default settings, middle intensities are brighter than either end of the palette.
        const double Frequency           = Colorization.IntensityCurveFrequency * PlasmaIntensity;
        const double Phase               = Colorization.IntensityCurvePhase;
        const double IntensityBrightness = Colorization.IntensityBrightnessBase + (Colorization.IntensityBrightnessSpan * std::sin(Frequency + Phase));

        // Separate the continuous loudness response from short transient flashes.
        const double AudioBrightness = Colorization.MinimumBrightness + (Colorization.LoudnessBrightness * LoudnessResponse) + (Colorization.PeakBrightness * _Audio.Peak);

        const double V = Clamp(_Theme.Brightness * AudioBrightness * IntensityBrightness);

        double R, G, B;

        HSVToRGB(H, S, V, R, G, B);

        // Apply frequency-specific RGB accents after HSV conversion.
        R = Clamp(R + _Theme.BassRed    * _Audio.Bass);
        G = Clamp(G + _Theme.MidGreen   * _Audio.Mid);
        B = Clamp(B + _Theme.TrebleBlue * _Audio.Treble);

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
    /// Generates a contiguous range of plasma rows.
    /// </summary>
    void effect_t::GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept
    {
        lastRow = std::min(lastRow, (uint32_t) _RasterHeight);

        for (uint32_t y = firstRow; y < lastRow; ++y)
        {
            const double PlasmaY = (2. * (double) y * work.InverseHeight - 1.) * work.CoordinateScale - _Interaction.Y;
            const double DetailRowPhase = PlasmaY * 4. + work.DetailPhase;
            const size_t PixelRowOffset = (size_t) (y * (uint32_t) _RasterWidth);

            auto * Row = &_Pixels[PixelRowOffset];

            for (uint32_t x = 0; x < (uint32_t) _RasterWidth; ++x, ++Row)
            {
                const double PlasmaX = (2. * (double) x * work.InverseWidth - 1.) * work.HorizontalScale - _Interaction.X + work.BalanceOffset;

                double PlasmaValue = std::sin(PlasmaX * work.HorizontalFrequency + work.HorizontalPhase);

                PlasmaValue += std::sin((PlasmaX + PlasmaY) * work.DiagonalFrequency - work.DiagonalPhase);

                const double OffsetFromCenterX = PlasmaX + work.RadialCenterX;
                const double OffsetFromCenterY = PlasmaY + work.RadialCenterY;
                const double DistanceFromCenter = std::sqrt(OffsetFromCenterX * OffsetFromCenterX + OffsetFromCenterY * OffsetFromCenterY);

                PlasmaValue += std::sin(DistanceFromCenter * work.RadialFrequency - work.RadialPhase);
                PlasmaValue += .65 * std::sin(DetailRowPhase + std::sin(PlasmaX * 2. + work.Time) * 1.5);

                if (work.HasImpulse)
                {
                    const double DistanceFromOrigin = std::sqrt((PlasmaX * PlasmaX) + (PlasmaY * PlasmaY));

                    PlasmaValue += _Interaction.Impulse * std::sin(DistanceFromOrigin * 18. - work.RipplePhase);
                }

                PlasmaValue = .5 + (.5 * std::sin(PlasmaValue * 1.35 + work.ColorPhase));

                *Row = Colorize(PlasmaValue);
            }
        }
    }
}

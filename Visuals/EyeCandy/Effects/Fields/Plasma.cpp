
/** $VER: Plasma.cpp (2026.10.20) **/

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
        constexpr uint32_t Dividers[] = { 5, 3, 2 };

        _RasterWidth  = std::max(64u, (uint32_t) (_Width  / (float) Dividers[(size_t) _Quality]));
        _RasterHeight = std::max(36u, (uint32_t) (_Height / (float) Dividers[(size_t) _Quality]));

        _Pixels.resize((size_t) (_RasterWidth * _RasterHeight));
    }

    /// <summary>
    /// Renders this instance.
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept
    {
        // Update the pixel matrix.
        {
            _Audio = _Analyzer.Analyze(chunk, deltaTime);// : (_Analyzer.Decay(DeltaTime), _Analyzer.GetAudioFeatures());

            {
                constexpr double SpeedFactors[] = { .45, .75, 1.15 };

                _AnimationTime += deltaTime * SpeedFactors[std::min((size_t) _Speed, _countof(SpeedFactors) - 1)];
            }

            _Interaction.Impulse *= std::exp(-2.6 * deltaTime);

            if (_AnimationTime > _NextTransitionTime)
            {
                ++_NextThemeIndex;

                if (_NextThemeIndex == theme_t::BuiltInCount)
                    _NextThemeIndex = 0;

                _NextTransitionTime = _AnimationTime + TransitionInterval;
            }

            if (_ThemeIndex != _NextThemeIndex)
            {
                _FromTheme = _Theme;
                _ToTheme   = theme_t::BuiltIn(_NextThemeIndex);

                _ThemeIndex = _NextThemeIndex;
                _TransitionTime = 0.;
            }

            _TransitionTime = std::min(_TransitionTime + (deltaTime / 0.85), 1.);

            if (_TransitionTime < 1.)
                _Theme = theme_t::Blend(_FromTheme, _ToTheme, _TransitionTime);

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
    /// Extracts six animation-friendly values from stereo audio: overall loudness, bass, midrange, treble, stereo balance, and peak amplitude
    /// </summary>
    audio_t analyzer_t::Analyze(const audio_chunk_impl & chunk, double deltaTime) noexcept
    {
        if (chunk.is_empty())
        {
            // Keep the visualizer moving naturally when no new audio is available.
            Decay(deltaTime);

            return _Audio;
        }

        const size_t FrameCount   = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();

        const unsigned SampleRate    = chunk.get_sample_rate();
        const unsigned ChannelConfig = chunk.get_channel_config();

        const audio_sample * Sample = chunk.get_data();

        // One-pole low-pass filter coefficients. _LowState tracks frequencies below roughly 180 Hz, while _HighState tracks frequencies below 2400 Hz.
        const double BassFilterCoefficient   = std::exp(-2. * Pi *  180. / (double) SampleRate);
        const double TrebleFilterCoefficient = std::exp(-2. * Pi * 2400. / (double) SampleRate);

        const double BassFilterInputCoefficient   = 1. - BassFilterCoefficient;
        const double TrebleFilterInputCoefficient = 1. - TrebleFilterCoefficient;

        const double InverseChannelCount = 1. / (double) ChannelCount;

        double TotalSignalEnergy = 0.;

        double BassEnergy        = 0.;
        double MidrangeEnergy    = 0.;
        double TrebleEnergy      = 0.;

        double LeftChannelEnergy  = 0.;
        double RightChannelEnergy = 0.;

        double PeakAmplitude = 0.;

        constexpr unsigned LeftChannelConfig =
            audio_chunk::channel_front_left        |
            audio_chunk::channel_back_left         |
            audio_chunk::channel_front_center_left |
            audio_chunk::channel_side_left         |
            audio_chunk::channel_top_front_left    |
            audio_chunk::channel_top_back_left;

        constexpr unsigned RightChannelConfig =
            audio_chunk::channel_front_right        |
            audio_chunk::channel_back_right         |
            audio_chunk::channel_front_center_right |
            audio_chunk::channel_side_right         |
            audio_chunk::channel_top_front_right    |
            audio_chunk::channel_top_back_right;

        // Cache the channel classification by channel index. A size_t mask avoids allocating memory in this noexcept function.
        constexpr size_t ChannelMaskBitCount = sizeof(size_t) * 8;

        size_t LeftChannelMask  = 0;
        size_t RightChannelMask = 0;

        const size_t CachedChannelCount = std::min(ChannelCount, ChannelMaskBitCount);

        for (size_t ChannelIndex = 0; ChannelIndex < CachedChannelCount; ++ChannelIndex)
        {
            const unsigned ChannelFlag = audio_chunk::g_extract_channel_flag(ChannelConfig, (unsigned) ChannelIndex);
            const size_t ChannelBit    = (size_t) 1 << ChannelIndex;

            if ((ChannelFlag & LeftChannelConfig) != 0)
            {
                LeftChannelMask |= ChannelBit;
            }
            else
            if ((ChannelFlag & RightChannelConfig) != 0)
            {
                RightChannelMask |= ChannelBit;
            }
        }

        for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
        {
            double MonoSample = 0.;

            for (size_t ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex, ++Sample)
            {
                const double ChannelSample       = (double) *Sample;
                const double ChannelSampleEnergy = ChannelSample * ChannelSample;

                TotalSignalEnergy += ChannelSampleEnergy;
                MonoSample        += ChannelSample;

                PeakAmplitude = std::max(PeakAmplitude, std::abs(ChannelSample));

                bool IsLeftChannel  = false;
                bool IsRightChannel = false;

                if (ChannelIndex < CachedChannelCount)
                {
                    const size_t ChannelBit = (size_t) 1 << ChannelIndex;

                    IsLeftChannel  = (LeftChannelMask  & ChannelBit) != 0;
                    IsRightChannel = (RightChannelMask & ChannelBit) != 0;
                }
                else
                {
                    // This fallback only applies to channel counts larger than the native size_t mask.
                    const unsigned ChannelFlag = audio_chunk::g_extract_channel_flag(ChannelConfig, (unsigned) ChannelIndex);

                    IsLeftChannel  = (ChannelFlag & LeftChannelConfig)  != 0;
                    IsRightChannel = (ChannelFlag & RightChannelConfig) != 0;
                }

                if (IsLeftChannel)
                {
                    LeftChannelEnergy += ChannelSampleEnergy;
                }
                else
                if (IsRightChannel)
                {
                    RightChannelEnergy += ChannelSampleEnergy;
                }
                else
                {
                    // Center, LFE and unknown channels have no left/right bias.
                    const double SharedChannelEnergy = ChannelSampleEnergy * .5;

                    LeftChannelEnergy  += SharedChannelEnergy;
                    RightChannelEnergy += SharedChannelEnergy;
                }
            }

            // Analyze frequency content using a mono downmix.
            MonoSample *= InverseChannelCount;

            // Update the persistent one-pole low-pass filter states.
            _LowState  = BassFilterInputCoefficient   * MonoSample + BassFilterCoefficient   * _LowState;
            _HighState = TrebleFilterInputCoefficient * MonoSample + TrebleFilterCoefficient * _HighState;

            // Split the mono signal into approximate frequency bands:
            // - bass     = output of the 180 Hz low-pass filter
            // - treble   = content removed by the 2400 Hz low-pass filter
            // - midrange = content remaining between both cutoff frequencies
            const double BassSample     = _LowState;
            const double TrebleSample   = MonoSample - _HighState;
            const double MidrangeSample = _HighState - _LowState;

            BassEnergy     += BassSample     * BassSample;
            MidrangeEnergy += MidrangeSample * MidrangeSample;
            TrebleEnergy   += TrebleSample   * TrebleSample;
        }

        const double InverseFrameCount  = 1. / (double) FrameCount;
        const double InverseSampleCount = InverseFrameCount * InverseChannelCount;

        audio_t NewAudio;

        // Convert accumulated signal energy to RMS amplitudes. The scale factors tune visual sensitivity.
        NewAudio.Loudness = Clamp(std::sqrt(TotalSignalEnergy * InverseSampleCount) * 3.4);

        NewAudio.Bass     = Clamp(std::sqrt(BassEnergy        * InverseFrameCount) * 5.2);
        NewAudio.Mid      = Clamp(std::sqrt(MidrangeEnergy    * InverseFrameCount) * 6.4);
        NewAudio.Treble   = Clamp(std::sqrt(TrebleEnergy      * InverseFrameCount) * 8.5);

        // Energy balance ranges approximately from -1 for fully left to +1 for fully right.
        NewAudio.Balance = (RightChannelEnergy - LeftChannelEnergy) / (LeftChannelEnergy + RightChannelEnergy + Epsilon);

        NewAudio.Peak = Clamp(PeakAmplitude);

        // Apply separate response speeds because each visual characteristic benefits from a different amount of motion and stability.
        _Audio.Loudness = Interpolate(_Audio.Loudness, NewAudio.Loudness, deltaTime, 10.);
        _Audio.Bass     = Interpolate(_Audio.Bass,     NewAudio.Bass,     deltaTime,  7.);
        _Audio.Mid      = Interpolate(_Audio.Mid,      NewAudio.Mid,      deltaTime,  9.);
        _Audio.Treble   = Interpolate(_Audio.Treble,   NewAudio.Treble,   deltaTime, 13.);
        _Audio.Balance  = Interpolate(_Audio.Balance,  NewAudio.Balance,  deltaTime,  5.);

        // PeakAmplitude tracks the largest absolute sample in the entire chunk. Unlike RMS loudness, this responds directly to sharp transients such as drum hits.
        // The smoothing logic gives peaks an immediate attack and an exponential release producing a stable peak-hold effect without hiding sudden transients.
        const double PeakRelease = std::exp(-2.8 * deltaTime);

        _Audio.Peak = std::max(NewAudio.Peak, _Audio.Peak * PeakRelease);

        return _Audio;
    }

    /// <summary>
    /// Decays the audio parameters.
    /// </summary>
    void analyzer_t::Decay(double deltaTime) noexcept
    {
        _Audio.Loudness *= std::exp(-2.4 * deltaTime);

        _Audio.Bass     *= std::exp(-2   * deltaTime);
        _Audio.Mid      *= std::exp(-2.5 * deltaTime);
        _Audio.Treble   *= std::exp(-3   * deltaTime);

        _Audio.Balance  *= std::exp(-2   * deltaTime);
        _Audio.Peak     *= std::exp(-2.8 * deltaTime);
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
        const double Hue =
            _Theme.BaseHue +
            _Theme.HueSpan   * PlasmaIntensity +
            _Theme.BassHue   * _Audio.Bass +
            _Theme.MidHue    * _Audio.Mid +
            _Theme.TrebleHue * _Audio.Treble;

        // Treble can increase saturation to make high-frequency activity appear sharper and more colorful.
        const double Saturation = Clamp(_Theme.Saturation + (Colorization.TrebleSaturation * _Audio.Treble));

        // Shape brightness across the plasma field. With the default settings, middle intensities are brighter than either end of the palette.
        const double Frequency = Colorization.IntensityCurveFrequency * PlasmaIntensity;
        const double Phase     = Colorization.IntensityCurvePhase;

        const double IntensityBrightness = Colorization.IntensityBrightnessBase + (Colorization.IntensityBrightnessSpan * std::sin(Frequency + Phase));

        // Separate the continuous loudness response from short transient flashes.
        const double AudioBrightness = Colorization.MinimumBrightness + (Colorization.LoudnessBrightness * LoudnessResponse) + (Colorization.PeakBrightness * _Audio.Peak);
        const double Brightness      = Clamp(_Theme.Brightness * AudioBrightness * IntensityBrightness);

        double Red, Green, Blue;

        HSVToRGB(Hue, Saturation, Brightness, Red, Green, Blue);

        // Apply frequency-specific RGB accents after HSV conversion.
        Red   = Clamp(Red   + _Theme.BassRed    * _Audio.Bass);
        Green = Clamp(Green + _Theme.MidGreen   * _Audio.Mid);
        Blue  = Clamp(Blue  + _Theme.TrebleBlue * _Audio.Treble);

        const uint32_t R = (uint32_t) ToRGB(Red);
        const uint32_t G = (uint32_t) ToRGB(Green);
        const uint32_t B = (uint32_t) ToRGB(Blue);

        return 0xFF000000u | (R << 16) | (G << 8) | B;
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

            for (uint32_t x = 0; x < (uint32_t) _RasterWidth; ++x)
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

                _Pixels[PixelRowOffset + x] = Colorize(PlasmaValue);
            }
        }
    }
}

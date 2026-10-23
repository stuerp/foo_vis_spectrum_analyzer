
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
        constexpr uint32_t Dividers[] = { 6, 4, 3 };

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
            _Audio = _Analyzer.Analyze(chunk, deltaTime);

            {
                constexpr double SpeedFactors[] = { .45, .75, 1.15 };

                const double AudioActivity = Clamp((_Audio.Loudness * .65) + (_Audio.Bass * .20) + (_Audio.Treble * .15));

                _AnimationTime += deltaTime * SpeedFactors[std::min((size_t) _Speed, _countof(SpeedFactors) - 1)] * AudioActivity;
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

            _TransitionTime = std::min(_TransitionTime + (deltaTime / .40), 1.);

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

        // Render the bitmap across the complete render target. Using the target dimensions here keeps the effect fullscreen
        // when the host switches display modes or resizes the visualization window without depending on the layout rectangle.
        {
            renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

            const auto Size = renderTarget->GetSize();
            const auto FullscreenRectangle = D2D1::RectF(0.f, 0.f, Size.width, Size.height);

            if (_BackBuffer)
                renderTarget->DrawBitmap(_BackBuffer.Get(), FullscreenRectangle, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
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
        NewAudio.Loudness = std::clamp(std::sqrt(TotalSignalEnergy * InverseSampleCount) * 3.4, 0., 1.);

        NewAudio.Bass     = std::clamp(std::sqrt(BassEnergy        * InverseFrameCount) * 5.2, 0., 1.);
        NewAudio.Mid      = std::clamp(std::sqrt(MidrangeEnergy    * InverseFrameCount) * 6.4, 0., 1.);
        NewAudio.Treble   = std::clamp(std::sqrt(TrebleEnergy      * InverseFrameCount) * 8.5, 0., 1.);

        // Energy balance ranges approximately from -1 for fully left to +1 for fully right.
        NewAudio.Balance = (RightChannelEnergy - LeftChannelEnergy) / (LeftChannelEnergy + RightChannelEnergy + Epsilon);

        NewAudio.Peak = std::clamp(PeakAmplitude, 0., 1.);

        // Apply separate response speeds because each visual characteristic benefits from a different amount of motion and stability.
        _Audio.Loudness = Interpolate(_Audio.Loudness, NewAudio.Loudness, deltaTime, 10);
        _Audio.Bass     = Interpolate(_Audio.Bass,     NewAudio.Bass,     deltaTime,  7);
        _Audio.Mid      = Interpolate(_Audio.Mid,      NewAudio.Mid,      deltaTime,  9);
        _Audio.Treble   = Interpolate(_Audio.Treble,   NewAudio.Treble,   deltaTime, 13);
        _Audio.Balance  = Interpolate(_Audio.Balance,  NewAudio.Balance,  deltaTime,  5);

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

        const double Hue =
            _Theme.BaseHue +
           (_Theme.HueSpan   * ((Fold * .72) + (Height * .18))) +
           (_Theme.BassHue   * _Audio.Bass) +
           (_Theme.MidHue    * _Audio.Mid) +
           (_Theme.TrebleHue * _Audio.Treble);

        const double Saturation = std::clamp((_Theme.Saturation * .78) + (.14 * Fold) + (.08 * _Audio.Treble), 0., 1.);
        const double Ambient    = .012 + (.055 * std::pow(std::clamp(_Audio.Loudness, 0., 1.), .6));
        const double Brightness = std::clamp(Ambient + (_Theme.Brightness * Intensity * (.56 + (.44 * Fold))), 0., 1.);

        double Red, Green, Blue;

        HSVToRGB(Hue, Saturation, Brightness, Red, Green, Blue);

        Red   = Clamp(Red   + (_Theme.BassRed    * _Audio.Bass)   + (.06 * Fold));
        Green = Clamp(Green + (_Theme.MidGreen   * _Audio.Mid)    + (.14 * Intensity));
        Blue  = Clamp(Blue  + (_Theme.TrebleBlue * _Audio.Treble) + (.10 * Fold));

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
    /// Generates a contiguous range of luminous aurora-curtain rows.
    /// </summary>
    void effect_t::GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept
    {
        lastRow = std::min(lastRow, (uint32_t) _RasterHeight);

        for (uint32_t y = firstRow; y < lastRow; ++y)
        {
            const double V = (double) y * work.InverseHeight;
            const size_t PixelRowOffset = (size_t) (y * (uint32_t) _RasterWidth);

            for (uint32_t x = 0; x < (uint32_t) _RasterWidth; ++x)
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

                _Pixels[PixelRowOffset + x] = Colorize(Intensity, BrightestFold, 1. - V);
            }
        }
    }
}

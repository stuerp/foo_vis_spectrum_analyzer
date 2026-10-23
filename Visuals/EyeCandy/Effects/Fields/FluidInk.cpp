
/** $VER: FluidInk.cpp (2026.10.22) **/

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
        constexpr uint32_t Dividers[] = { 7, 4, 2 };

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

                const double AudioActivity = Clamp((_Audio.Loudness * .55) + (_Audio.Bass * .25) + (_Audio.Mid * .10) + (_Audio.Treble * .10));

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
        NewAudio.Balance = (RightChannelEnergy - LeftChannelEnergy) /
                           (LeftChannelEnergy + RightChannelEnergy + Epsilon);

        NewAudio.Peak = Clamp(PeakAmplitude);

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

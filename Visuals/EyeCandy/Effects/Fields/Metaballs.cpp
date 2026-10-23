
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
        constexpr uint32_t Dividers[] = { 7, 5, 3 };

        _RasterWidth  = std::max(64u, (uint32_t) (_Width  / (float) Dividers[(size_t) _Quality]));
        _RasterHeight = std::max(36u, (uint32_t) (_Height / (float) Dividers[(size_t) _Quality]));

        _Pixels.resize((size_t) (_RasterWidth * _RasterHeight));
    }

    /// <summary>
    /// Renders this instance.
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        // Update the pixel matrix.
        {
            _Audio = _Analyzer.Analyze(chunk, deltaTime);

            {
                constexpr double SpeedFactors[] = { .45, .75, 1.15 };
                const size_t SpeedIndex = std::min((size_t) _Speed, _countof(SpeedFactors) - 1);

                // Keep geometry and theme timing local to this effect. The bitmap can be
                // recreated or reset without resetting the phase that moves the blobs.
                _MotionTime += deltaTime * SpeedFactors[SpeedIndex];
                _ThemeTime  += deltaTime;

                // Keep the trigonometric argument bounded during long-running sessions.
                constexpr double WrapTime = 2. * Pi * 1024.;
                if (_MotionTime >= WrapTime)
                    _MotionTime = std::fmod(_MotionTime, 2. * Pi);
            }

            _Interaction.Impulse *= std::exp(-2.6 * deltaTime);

            if (_ThemeTime > _NextTransitionTime)
            {
                ++_NextThemeIndex;

                if (_NextThemeIndex == theme_t::BuiltInCount)
                    _NextThemeIndex = 0;

                _NextTransitionTime = _ThemeTime + TransitionInterval;
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

            Update(_MotionTime);
        }

        // Render the bitmap.
        {
            _BackBuffer->CopyFromMemory(nullptr, _Pixels.data(), _RasterWidth * BytesPerPixel);

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

        double TotalSignalEnergy  = 0.;

        double BassEnergy         = 0.;
        double MidrangeEnergy     = 0.;
        double TrebleEnergy       = 0.;

        double LeftChannelEnergy  = 0.;
        double RightChannelEnergy = 0.;

        double PeakAmplitude      = 0.;

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
            // - Bass     = output of the 180 Hz low-pass filter
            // - Treble   = content removed by the 2400 Hz low-pass filter
            // - Midrange = content remaining between both cutoff frequencies
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

        const double Hue = _Theme.BaseHue + (_Theme.HueSpan * (.12 + .72 * interior)) + (_Theme.BassHue * _Audio.Bass) + (_Theme.MidHue * _Audio.Mid) + (_Theme.TrebleHue * _Audio.Treble);

        const double Saturation = Clamp(_Theme.Saturation + .08 - .22 * edge);
        const double Brightness = Clamp(glow + surface * (_Theme.Brightness * (.36 + .50 * interior)) + .28 * edge);

        double Red, Green, Blue;

        HSVToRGB(Hue, Saturation, Brightness, Red, Green, Blue);

        Red   = Clamp(Red   + surface * _Theme.BassRed    * _Audio.Bass   + .12 * edge);
        Green = Clamp(Green + surface * _Theme.MidGreen   * _Audio.Mid    + .15 * edge);
        Blue  = Clamp(Blue  + surface * _Theme.TrebleBlue * _Audio.Treble + .20 * edge);

        const uint32_t R  = (uint32_t) ToRGB(Red);
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

            for (uint32_t x = 0; x < (uint32_t) _RasterWidth; ++x)
            {
                const double U = ((double) x + .5) * work.InverseWidth;

                double Field = 0.;

                for (size_t i = 0; i < BlobCount; ++i)
                {
                    const double DX = (U - work.BlobX[i]) * work.AspectRatio;
                    const double DY = V - work.BlobY[i];
                    const double DistanceSquared = DX * DX + DY * DY + 1e-8;
                    const double RadiusSquared = work.BlobRadius[i] * work.BlobRadius[i];

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

                _Pixels[PixelRowOffset + x] = Colorize(Surface, Interior, Edge, Glow);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        const auto Size = D2D1::SizeU(_RasterWidth, _RasterHeight); // Can be different than the render target size.

        if ((_BackBuffer == nullptr) || (_BackBuffer->GetPixelSize().width != Size.width) || (_BackBuffer->GetPixelSize().height != Size.height))
        {
            _BackBuffer.Reset();

            renderTarget->CreateBitmap(Size, nullptr, 0, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), _BackBuffer.GetAddressOf());
        }

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _BackBuffer.Reset();
    }
}

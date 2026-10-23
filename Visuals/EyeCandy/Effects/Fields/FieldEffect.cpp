
/** $VER: FieldEffect.cpp (2026.10.23) - Base class for a field effect */

#include <pch.h>

#include "FieldEffect.h"

#include <algorithm>
#include <cmath>

namespace Field
{
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

        // During track hand-off a chunk can be structurally empty even when
        // is_empty() did not catch it. Do not divide by zero or construct invalid
        // filter coefficients from that transient chunk.
        if (FrameCount == 0 || ChannelCount == 0 || SampleRate == 0 || Sample == nullptr)
        {
            Decay(deltaTime);
            return _Audio;
        }

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

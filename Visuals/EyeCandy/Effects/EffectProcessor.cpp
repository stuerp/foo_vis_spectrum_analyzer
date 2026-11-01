
/** $VER: EffectProcessor.cpp (2026.11.01) - Audio processor for Eye Candy effects **/

#include <pch.h>

#include "EffectProcessor.h"
#include "Effect.h"

#include <algorithm>
#include <cmath>

namespace EyeCandy
{
    /// <summary>
    /// Extracts six animation-friendly values from stereo audio: overall loudness, bass, midrange, treble, stereo balance, and peak amplitude
    /// </summary>
    void processor_t::Process(const audio_chunk & chunk, double deltaTime) noexcept
    {
        if (chunk.is_empty())
        {
            // Keep the visualizer moving naturally when no new audio is available.
            Decay(deltaTime);

            return;
        }

        const size_t FrameCount   = chunk.get_sample_count();
        const size_t ChannelCount = chunk.get_channel_count();

        const unsigned SampleRate    = chunk.get_sample_rate();
        const unsigned ChannelConfig = chunk.get_channel_config();

        const audio_sample * Sample = chunk.get_data();

        // During track hand-off a chunk can be structurally empty even when is_empty() did not catch it.
        // Do not divide by zero or construct invalid filter coefficients from that transient chunk.
        if (FrameCount == 0 || ChannelCount == 0 || SampleRate == 0 || Sample == nullptr)
        {
            // Keep the visualizer moving naturally when no new audio is available.
            Decay(deltaTime);

            return;
        }

        // Calculate the one-pole low-pass filter coefficients. _LowState tracks frequencies below roughly 220 Hz, while _HighState tracks frequencies below 2400 Hz.
        const double BassCoefficient   = std::exp(-TwoPi * BassCutoff / (double) SampleRate);
        const double TrebleCoefficient = std::exp(-TwoPi * MidCutoff  / (double) SampleRate);

        const double BassInputCoefficient   = 1. - BassCoefficient;
        const double TrebleInputCoefficient = 1. - TrebleCoefficient;

        const double ChannelScale = 1. / (double) ChannelCount;

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

        // Process all frames in the chunk.
        for (size_t i = 0; i < FrameCount; ++i)
        {
            double Mono = 0.;

            // Prcoess all samples in the frame.
            for (size_t j = 0; j < ChannelCount; ++j, ++Sample)
            {
                const double ChannelSample       = (double) *Sample;
                const double ChannelSampleEnergy = ChannelSample * ChannelSample;

                TotalSignalEnergy += ChannelSampleEnergy;
                Mono        += ChannelSample;

                PeakAmplitude = std::max(PeakAmplitude, std::abs(ChannelSample));

                bool IsLeftChannel  = false;
                bool IsRightChannel = false;

                if (j < CachedChannelCount)
                {
                    const size_t ChannelBit = (size_t) 1 << j;

                    IsLeftChannel  = (LeftChannelMask  & ChannelBit) != 0;
                    IsRightChannel = (RightChannelMask & ChannelBit) != 0;
                }
                else
                {
                    // This fallback only applies to channel counts larger than the native size_t mask.
                    const unsigned ChannelFlag = audio_chunk::g_extract_channel_flag(ChannelConfig, (unsigned) j);

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
            Mono *= ChannelScale;

            // Update the persistent one-pole low-pass filter states.
            _LowState  = (BassInputCoefficient   * Mono) + (BassCoefficient   * _LowState);
            _HighState = (TrebleInputCoefficient * Mono) + (TrebleCoefficient * _HighState);

            // Split the mono signal into approximate frequency bands.
            const double Bass   = _LowState;
            const double Treble = Mono - _HighState;
            const double Mid    = _HighState - _LowState;

            BassEnergy     += Bass   * Bass;
            MidrangeEnergy += Mid    * Mid;
            TrebleEnergy   += Treble * Treble;
        }

        const double FramesScale  = 1. / (double) FrameCount;
        const double SamplesScale = FramesScale * ChannelScale;

        // Convert accumulated signal energy to RMS amplitudes. The scale factors tune visual sensitivity.
        double NewLoudness = Clamp(std::sqrt(TotalSignalEnergy * SamplesScale) * 3.4);

        double NewBass     = Clamp(std::sqrt(BassEnergy        * FramesScale) * 5.2);
        double NewMid      = Clamp(std::sqrt(MidrangeEnergy    * FramesScale) * 6.4);
        double NewTreble   = Clamp(std::sqrt(TrebleEnergy      * FramesScale) * 8.5);

        // Energy balance ranges approximately from -1 for fully left to +1 for fully right.
        double NewBalance  = (RightChannelEnergy - LeftChannelEnergy) / (LeftChannelEnergy + RightChannelEnergy + Epsilon);
        double NewPeak     = Clamp(PeakAmplitude);

        // Apply separate response speeds because each visual characteristic benefits from a different amount of motion and stability.
        _Loudness = Interpolate(_Loudness, NewLoudness, deltaTime, 10);

        _Bass     = Interpolate(_Bass,     NewBass,     deltaTime,  7);
        _Mid      = Interpolate(_Mid,      NewMid,      deltaTime,  9);
        _Treble   = Interpolate(_Treble,   NewTreble,   deltaTime, 13);

        _Balance  = Interpolate(_Balance,  NewBalance,  deltaTime,  5);

        // PeakAmplitude tracks the largest absolute sample in the entire chunk. Unlike RMS loudness, this responds directly to sharp transients such as drum hits.
        // The smoothing logic gives peaks an immediate attack and an exponential release producing a stable peak-hold effect without hiding sudden transients.
        const double PeakRelease = std::exp(-2.8 * deltaTime);

        _Peak = std::max(NewPeak, _Peak * PeakRelease);
    }

    /// <summary>
    /// Decays the audio parameters.
    /// </summary>
    void processor_t::Decay(double deltaTime) noexcept
    {
        _Loudness *= std::exp(-2.4 * deltaTime);

        _Bass     *= std::exp(-2   * deltaTime);
        _Mid      *= std::exp(-2.5 * deltaTime);
        _Treble   *= std::exp(-3   * deltaTime);

        _Balance  *= std::exp(-2   * deltaTime);
        _Peak     *= std::exp(-2.8 * deltaTime);
    }
}


/** $VER: EffectProcessor.cpp (2026.11.11) - Audio processor for Eye Candy effects **/

#include <pch.h>

#include "EffectProcessor.h"
#include "Effect.h"

#include <algorithm>
#include <cmath>

namespace EyeCandy
{
    /// <summary>
    /// Resets the measurements of this instance.
    /// </summary>
    void effect_processor_t::Reset() noexcept
    {
        _HasAudio = false;

        _SampleRate = 0.;

        _Peak                   = 0.;
        _RMS                    = 0.;

        _CrestFactor            = 0.;
        _Correlation            = 0.;
        _BassCorrelation        = 0.;
        _StereoWidth            = 0.;
        _PhaseEnergy            = 0.;
        _ChannelBalance         = 0.;

        _SpectralFlux           = 0.;
        _SpectralCentroid       = 0.;
        _SpectralFlatness       = 0.;

        _BassLevel              = 0.;
        _MidLevel               = 0.;
        _TrebleLevel            = 0.;

        _BeatAverage            = 0.;
        _BeatEnvelope           = 0.;
        _BeatPulse              = 0.;
        _BeatCooldown           = 0.;

        _PreviousBeatEnvelope   = 0.;

        for (auto & Band : _Bands)
        {
            Band.Energy         = 0.;
            Band.SmoothedEnergy = 0.;
            Band.PreviousEnergy = 0.;
        }

        // One-pole low-pass filter.
        _BassFilterLeft         = 0.;
        _BassFilterRight        = 0.;
        _BassFilterLeftChannel  = SIZE_MAX;
        _BassFilterRightChannel = SIZE_MAX;
        _BassFilterSampleRate   = 0.;
        _BassFilterPrimed       = false;

        // One-pole low-pass filter.
        _SubBassState = 0.;
        _BassState    = 0.;
        _MidState     = 0.;

        _IsFilterPrimed = false;
    }

    /// <summary>
    /// Extracts six animation-friendly values from stereo audio: overall loudness, bass, midrange, treble, stereo balance, and peak amplitude
    /// </summary>
    void effect_processor_t::Process(const audio_chunk & chunk, double deltaTime) noexcept
    {
        if (chunk.is_empty())
        {
            // Keep the visualizer moving naturally when no new audio is available.
            Decay(deltaTime);

            return;
        }

        const audio_sample * const Frames = chunk.get_data();

        const auto FrameCount    = chunk.get_sample_count();
        const auto ChannelCount  = chunk.get_channel_count();
        const auto ChannelConfig = chunk.get_channel_config();
        const auto SampleRate    = (double) chunk.get_sample_rate();

        // Calculate the one-pole low-pass filter coefficients. _LowState tracks frequencies below roughly 220 Hz, while _HighState tracks frequencies below 2400 Hz.
        const double BassCoefficient = std::exp(-TwoPi * BassCutoff / SampleRate);
        const double MidCoefficient  = std::exp(-TwoPi * MidCutoff  / SampleRate);

        const double BassInputCoefficient = 1. - BassCoefficient;
        const double MidInputCoefficient  = 1. - MidCoefficient;

        const double ChannelScale = 1. / (double) ChannelCount;

        double TotalPower         = 0.;

        double BassEnergy         = 0.;
        double MidEnergy          = 0.;
        double TrebleEnergy       = 0.;

        double BalancePower       = 0.;
        double MidChannelEnergy   = 0.;
        double SideChannelEnergy  = 0.;

        double PeakAmplitude      = 0.;

        constexpr unsigned LeftChannelConfig  = audio_chunk::channel_front_left  | audio_chunk::channel_back_left  | audio_chunk::channel_front_center_left  | audio_chunk::channel_side_left  | audio_chunk::channel_top_front_left  | audio_chunk::channel_top_back_left;
        constexpr unsigned RightChannelConfig = audio_chunk::channel_front_right | audio_chunk::channel_back_right | audio_chunk::channel_front_center_right | audio_chunk::channel_side_right | audio_chunk::channel_top_front_right | audio_chunk::channel_top_back_right;

        size_t LeftChannelMask  = 0;
        size_t RightChannelMask = 0;

        {
            for (size_t i = 0; i < ChannelCount; ++i)
            {
                const unsigned ChannelFlag = audio_chunk::g_extract_channel_flag(ChannelConfig, (unsigned) i);

                const size_t ChannelBit = (size_t) 1 << i;

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
        }

        // Process all frames in the chunk.
        const audio_sample * Frame = Frames;

        for (size_t i = 0; i < FrameCount; ++i)
        {
            double Mono = 0.;

            double LeftSample  = 0.;
            double RightSample = 0.;

            double LeftWeight  = 0.;
            double RightWeight = 0.;

            // Process all samples in the frame.
            for (size_t j = 0; j < ChannelCount; ++j)
            {
                const double Sample = (double) *Frame++;

                const double Power = Sample * Sample;

                TotalPower += Power;
                Mono       += Sample;

                PeakAmplitude = std::max(PeakAmplitude, std::abs(Sample));

                bool IsLeftChannel  = false;
                bool IsRightChannel = false;

                const size_t ChannelBit = (size_t) 1 << j;

                IsLeftChannel  = (LeftChannelMask  & ChannelBit) != 0;
                IsRightChannel = (RightChannelMask & ChannelBit) != 0;

                if (IsLeftChannel)
                {
                    BalancePower -= Power;

                    LeftSample += Sample;
                    LeftWeight += 1.;
                }
                else
                if (IsRightChannel)
                {
                    BalancePower += Power;

                    RightSample += Sample;
                    RightWeight += 1.;
                }
                else
                {
                    // Center, LFE and unknown channels are shared equally and therefore contribute to mid energy without creating artificial side energy or balance.
                    LeftSample  += Sample * .5;
                    RightSample += Sample * .5;

                    LeftWeight  += .5;
                    RightWeight += .5;
                }
            }

            // Form normalized left/right composites, then accumulate mid/side energy.
            LeftSample  /= std::max(LeftWeight,  Epsilon);
            RightSample /= std::max(RightWeight, Epsilon);

            const double MidSample  = (LeftSample + RightSample) * 0.5;
            const double SideSample = (LeftSample - RightSample) * 0.5;

            MidChannelEnergy  += MidSample  * MidSample;
            SideChannelEnergy += SideSample * SideSample;

            // Analyze frequency content using a mono downmix.
            Mono *= ChannelScale;

            // Update the persistent one-pole low-pass filter states.
            if (!_IsFilterPrimed)
            {
                _BassState = Mono;
                _MidState  = Mono;

                _IsFilterPrimed = true;
            }
            else
            {
                _BassState = (BassInputCoefficient * Mono) + (BassCoefficient * _BassState);
                _MidState  = (MidInputCoefficient  * Mono) + (MidCoefficient  * _MidState);
            }

            // Split the mono signal into approximate frequency bands.
            const double Bass   = _BassState;
            const double Treble = Mono - _MidState;
            const double Mid    = _MidState - _BassState;

            // Accumulate the energy for each band.
            BassEnergy   += Bass   * Bass;
            MidEnergy    += Mid    * Mid;
            TrebleEnergy += Treble * Treble;
        }

        const double FramesScale  = 1. / (double) FrameCount;
        const double SamplesScale = FramesScale * ChannelScale;

        // Convert accumulated signal energy to RMS amplitudes. The scale factors tune visual sensitivity.
        double NewLoudness    = Clamp(std::sqrt(TotalPower * SamplesScale) * 3.4);

        double NewBassLevel   = Clamp(std::sqrt(BassEnergy   * FramesScale) * 5.2);
        double NewMidLevel    = Clamp(std::sqrt(MidEnergy    * FramesScale) * 6.4);
        double NewTrebleLevel = Clamp(std::sqrt(TrebleEnergy * FramesScale) * 8.5);

        // Energy balance ranges approximately from -1 for fully left to +1 for fully right.
        double NewBalance = BalancePower / (TotalPower + Epsilon);

        const double MidRMS  = std::sqrt(MidChannelEnergy  * FramesScale);
        const double SideRMS = std::sqrt(SideChannelEnergy * FramesScale);

        double NewStereoWidth = Clamp(SideRMS / (MidRMS + SideRMS + Epsilon));
        double NewPeak        = Clamp(PeakAmplitude);

        // Apply separate response speeds because each visual characteristic benefits from a different amount of motion and stability.
        _Loudness     = Interpolate(_Loudness,    NewLoudness,    deltaTime, 10);

        _BassLevel    = Interpolate(_BassLevel,   NewBassLevel,   deltaTime,  7);
        _MidLevel     = Interpolate(_MidLevel,    NewMidLevel,    deltaTime,  9);
        _TrebleLevel  = Interpolate(_TrebleLevel, NewTrebleLevel, deltaTime, 13);

        _Balance      = Interpolate(_Balance,     NewBalance,     deltaTime,  5);
        _StereoWidth  = Interpolate(_StereoWidth, NewStereoWidth, deltaTime,  5.5);

        // PeakAmplitude tracks the largest absolute sample in the entire chunk. Unlike RMS loudness, this responds directly to sharp transients such as drum hits.
        // The smoothing logic gives peaks an immediate attack and an exponential release producing a stable peak-hold effect without hiding sudden transients.
        const double PeakRelease = std::exp(-2.8 * deltaTime);

        _Peak = std::max(NewPeak, _Peak * PeakRelease);
    }

    /// <summary>
    /// Decays the audio parameters.
    /// </summary>
    void effect_processor_t::Decay(double deltaTime) noexcept
    {
        _Loudness     *= std::exp(-2.4 * deltaTime);

        _BassLevel    *= std::exp(-2.0 * deltaTime);
        _MidLevel     *= std::exp(-2.5 * deltaTime);
        _TrebleLevel  *= std::exp(-3.0 * deltaTime);

        _Balance      *= std::exp(-2.0 * deltaTime);
        _StereoWidth  *= std::exp(-2.2 * deltaTime);
        _Peak         *= std::exp(-2.8 * deltaTime);
    }
}


/** $VER: EffectProcessor.cpp (2026.11.15) - Audio processor for Eye Candy effects **/

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

        _IsFilterPrimed = false;
    }

    /// <summary>
    /// Extracts animation-friendly values from stereo audio.
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

        if (_SampleRate != (double) chunk.get_sample_rate())
        {
            _SampleRate = (double) chunk.get_sample_rate();

            _IsFilterPrimed = false;
        }

        if (_ChannelPower.size() != ChannelCount)
        {
            _ChannelPower.resize(ChannelCount); // Total squared amplitude in a channel.

            _BassState.resize(ChannelCount);
            _MidState.resize(ChannelCount);

            _IsFilterPrimed = false;
        }

        std::fill(_ChannelPower.begin(), _ChannelPower.end(), 0.);

        if ((_ChannelConfig != ChannelConfig) || (ChannelCount != ChannelCount))
        {
            DetermineChannelMasks(ChannelConfig, ChannelCount);

            _ChannelConfig = ChannelConfig;
            _ChannelCount  = ChannelCount;
        }

        // Initialize the energy accumulators and per-channel filter states.
        double Peak = 0.; // Largest absolute sample value.

        double TotalPower           = 0.; // Squared amplitude across all channels.
        double TotalHorizontalPower = 0.;
        double BalancePower         = 0.;

        double CenterPower  = 0.; // Energy common to the channels. (Mid energy)
        double SpatialPower = 0.; // Energy representing differences between channels. (Side energy)
        double CrossProduct = 0.; // Pairwise channel similarity.

        double BassEnergy   = 0.;
        double MidEnergy    = 0.;
        double TrebleEnergy = 0.;

        // Calculate the one-pole low-pass filter coefficients. _BassState tracks frequencies below roughly 220 Hz, while _MidState tracks frequencies below 2400 Hz.
        const double BassCoefficient = std::exp(-TwoPi * BassCutoff / _SampleRate);
        const double MidCoefficient  = std::exp(-TwoPi * MidCutoff  / _SampleRate);

        const double BassInputCoefficient = 1. - BassCoefficient;
        const double MidInputCoefficient  = 1. - MidCoefficient;

        // Process all frames.
        const double InvFrameCount   = 1. / (double) FrameCount;
        const double InvChannelCount = 1. / (double) ChannelCount;

        const double SamplesScale = InvFrameCount * InvChannelCount;

        const audio_sample * Frame = Frames;

        for (size_t i = 0; i < FrameCount; ++i)
        {
            double Mono = 0.;

            double FramePower = 0.;
            double LeftPower  = 0.;
            double RightPower = 0.;

            // Process all samples in the frame.
            for (size_t j = 0, ChannelBit = 1; j < ChannelCount; ++j, ChannelBit <<= 1)
            {
                const double Sample = (double) *Frame++;
                const double Power  = Sample * Sample;

                Mono       += Sample;
                FramePower += Power;

                _ChannelPower[j] += Power;

                // Calculate peak level.
                Peak = std::max(Peak, std::abs(Sample));

                if ((_LeftChannelMask & ChannelBit) != 0)
                    LeftPower += Power;
                else
                if ((_RightChannelMask & ChannelBit) != 0)
                    RightPower += Power;

                // Update the persistent per-channel one-pole low-pass filter states.
                if (!_IsFilterPrimed)
                {
                    _BassState[j] = Sample;
                    _MidState[j]  = Sample;
                }
                else
                {
                    _BassState[j] = (BassInputCoefficient * Sample) + (BassCoefficient * _BassState[j]);
                    _MidState[j]  = (MidInputCoefficient  * Sample) + (MidCoefficient  * _MidState[j]);
                }

                // Split the channel signal into approximate frequency bands.
                const double Bass   = _BassState[j];
                const double Treble = Sample - _MidState[j];
                const double Mid    = _MidState[j] - _BassState[j];

                // Accumulate the energy for each band. Filtering each channel independently prevents anti-phase channel content from cancelling.
                BassEnergy   += Bass   * Bass;
                MidEnergy    += Mid    * Mid;
                TrebleEnergy += Treble * Treble;
            }

            _IsFilterPrimed = true;

            const double HorizontalPower = LeftPower + RightPower;

            TotalPower           += FramePower;
            TotalHorizontalPower += HorizontalPower;
            BalancePower         += RightPower - LeftPower;

            // Center contains the signal common to all channels. Spatial energy contains the remaining channel differences after the center component has been removed.
            const double Center        = Mono * InvChannelCount;
            const double CenterSquared = Center * Center;

            CenterPower += CenterSquared;

            SpatialPower += std::max(0., (FramePower * InvChannelCount) - CenterSquared);

            // The identity below accumulates the sum of all pairwise channel products without requiring a nested channel loop for every frame.
            CrossProduct += 0.5 * ((Mono * Mono) - FramePower);
        }

        // PeakAmplitude tracks the largest absolute sample in the entire chunk. Unlike RMS loudness, this responds directly to sharp transients such as drum hits.
        // The smoothing logic gives peaks an immediate attack and an exponential release producing a stable peak-hold effect without hiding sudden transients.
        {
            const double NewPeak = Clamp(Peak);

            const double PeakRelease = std::exp(-2.8 * deltaTime);

            _Peak = std::max(NewPeak, _Peak * PeakRelease);
        }

        // Calculate the RMS across all channels.
        {
            _RMS = std::sqrt(TotalPower * SamplesScale);
        }

        {
            const double NewLoudness = Clamp(_RMS * 3.4);

            _Loudness = Interpolate(_Loudness, NewLoudness, deltaTime, 10.);
        }

        // Horizontal energy balance ranges from -1 for entirely left to +1 for entirely right. Non-horizontal channels are ignored.
        {
            const double NewBalance = (TotalHorizontalPower > Epsilon) ? std::clamp(BalancePower / TotalHorizontalPower, -1., 1.) : 0.;

            _Balance = Interpolate(_Balance, NewBalance, deltaTime, 5.);
        }

        {
            const double NewBassLevel   = Clamp(std::sqrt(BassEnergy   * SamplesScale) * 5.2);
            const double NewMidLevel    = Clamp(std::sqrt(MidEnergy    * SamplesScale) * 6.4);
            const double NewTrebleLevel = Clamp(std::sqrt(TrebleEnergy * SamplesScale) * 8.5);

            double NewStereoWidth = 0.;
            double NewCorrelation = 0.;
            double NewPhaseEnergy = 0.;

            if (TotalPower > Epsilon)
            {
                // Calculate the stereo width: Side-to-mid ratio: 0.0: identical channels, fully mono; around 1.0: side energy equals mid energy; above 1.0: more side energy than mid energy; 2.0: maximum displayed width after clamping.
                {
                    if (CenterPower > Epsilon)
                        NewStereoWidth = std::sqrt(SpatialPower / CenterPower);
                    else
                        NewStereoWidth = (SpatialPower > Epsilon) ? 2. : 0.;

                    NewStereoWidth = std::clamp(NewStereoWidth, 0., 2.);
                }

                // Calculate the correlation: +1: channels have the same shape and polarity; 0: channels are unrelated or orthogonal over the analysis window; -1: channels have the same shape but opposite polarity.
                {
                    double CorrelationScale = 0.;

                    // Calculate the normalization factor.
                    for (size_t i = 0; i < ChannelCount; ++i)
                    {
                        const double FirstChannelPower = _ChannelPower[i];

                        for (size_t j = i + 1; j < ChannelCount; ++j)
                        {
                            CorrelationScale += std::sqrt(FirstChannelPower * _ChannelPower[j]);
                        }
                    }

                    NewCorrelation = (CorrelationScale > Epsilon) ? std::clamp(CrossProduct / CorrelationScale, -1., 1.) : 0.;
                }

                // Calculate the phase energy. Identical channels produce 0.0; one active channel produces 0.5 for stereo; opposite-polarity channels produce 1.0; unrelated equal-power stereo channels produce approximately 0.5.
                NewPhaseEnergy = Clamp(SpatialPower * (double) ChannelCount / TotalPower);
            }

            // Apply separate response speeds because each visual characteristic benefits from a different amount of motion and stability.
            _BassLevel   = Interpolate(_BassLevel,   NewBassLevel,   deltaTime, 7.);
            _MidLevel    = Interpolate(_MidLevel,    NewMidLevel,    deltaTime, 9.);
            _TrebleLevel = Interpolate(_TrebleLevel, NewTrebleLevel, deltaTime, 13.);

            _StereoWidth = Interpolate(_StereoWidth, NewStereoWidth, deltaTime, 5.5);
            _Correlation = Interpolate(_Correlation, NewCorrelation, deltaTime, 7.);
            _PhaseEnergy = Interpolate(_PhaseEnergy, NewPhaseEnergy, deltaTime, 8.);
        }
    }

    /// <summary>
    /// Decays the audio parameters.
    /// </summary>
    void effect_processor_t::Decay(double deltaTime) noexcept
    {
        _Peak         *= std::exp(-2.8 * deltaTime);
        _RMS          *= std::exp(-2.8 * deltaTime);

        _Loudness     *= std::exp(-2.4 * deltaTime);
        _Balance      *= std::exp(-2.0 * deltaTime);

        _BassLevel    *= std::exp(-2.0 * deltaTime);
        _MidLevel     *= std::exp(-2.5 * deltaTime);
        _TrebleLevel  *= std::exp(-3.0 * deltaTime);

        _StereoWidth  *= std::exp(-2.2 * deltaTime);
        _Correlation  *= std::exp(-2.2 * deltaTime);
        _PhaseEnergy  *= std::exp(-2.2 * deltaTime);
    }
}

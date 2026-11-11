
/** $VER: EffectProcessor.h (2026.11.09) - Audio processor for Eye Candy effects **/

#pragma once

#include <SDK/audio_chunk.h>

#include <array>

namespace EyeCandy
{
    class effect_processor_t
    {
    public:
        void Process(const audio_chunk & chunk, double deltaTime) noexcept;
        void Decay(double deltaTime) noexcept;

        void Reset() noexcept;

    protected:
        bool _HasAudio = false;

        std::vector<double> _Samples;
        double _SampleRate = 44100.;

        double _Peak                    = 0.; // [ 0, 1], Overall peak level. The loudest individual mono sample in a chunk. It indicates how close the signal gets to full scale.
        double _RMS                     = 0.; // [ 0, 1], Full-band mono RMS loudness.

        double _CrestFactor             = 0.; // in dB
        double _Correlation             = 0.; // [-1, 1], +1: channels have the same shape and polarity; 0: channels are unrelated or orthogonal over the analysis window; -1: channels have the same shape but opposite polarity.
        double _BassCorrelation         = 0.; // [-1, 1];
        double _StereoWidth             = 0.; // [ 0, 2], Side-to-mid ratio: 0.0: identical channels, fully mono / Around 1.0: side energy equals mid energy / Above 1.0: more side energy than mid energy / 2.0: maximum displayed width after clamping.
        double _PhaseEnergy             = 0.; // [ 0, 1], L = R: 0.0 / one active channel: 0.5 / L = -R: 1.0 / unrelated equal-power channels: approximately 0.5.
        double _ChannelBalance          = 0.; // in dB

        double _SpectralFlux            = 0.; // [ 0, 1], How much the frequency spectrum increased since the previous call. Responds when spectral energy increases such as during drum hits, note attacks, sudden treble bursts and new musical events.
        double _SpectralCentroid        = 0.;
        double _SpectralFlatness        = 0.;

        double _Loudness                = 0.; // [ 0, 1], Perceived full-band loudness derived from RMS energy and scaled for visual sensitivity. Can be use to control the response to perceived volume.

        double _BassLevel               = 0.; // [ 0, 1], Combined bass energy
        double _MidLevel                = 0.; // [ 0, 1], Combined midrange energy
        double _TrebleLevel             = 0.; // [ 0, 1], Combined treble energy

        double _Balance                 = 0.; // [-1, 1], Left-to-right energy balance: -1.0: fully left / 0.0: centered or equal energy / +1.0: fully right.
        double _StereoBalance           = 0.; // [-1, 1], Left-to-right energy balance: -1.0: fully left / 0.0: centered or equal energy / +1.0: fully right.

    protected:
        // One-pole low-pass filter state
        double _BassFilterLeft          = 0.; // Left bass filter state
        double _BassFilterRight         = 0.; // Right bass filter state
        size_t _BassFilterLeftChannel   = SIZE_MAX;
        size_t _BassFilterRightChannel  = SIZE_MAX;
        double _BassFilterSampleRate    = 0.;
        bool   _BassFilterPrimed        = false;

        struct band_t
        {
            double Energy         = 0.; // Energy at logarithmically spaced frequencies. (0-3 = bass, 4-8 = midrange, 9-11 = treble)
            double SmoothedEnergy = 0.;
            double PreviousEnergy = 0.;
            double Phase          = 0.;
        };

        static constexpr size_t BandCount = 12;
        static constexpr double InverseBandCount = 1. / (BandCount - 1);

        static_assert(BandCount > 1);

        std::array<band_t, BandCount> _Bands {};
        std::array<double, BandCount> _PitchClasses {};

    protected:
        double _SmoothedPeak            = 0.; // [ 0, 1]
        double _SmoothedRMS             = 0.; // [ 0, 1]
        double _SmoothedStereoWidth     = 0.; // [ 0, 1]
        double _SmoothedCorrelation     = 1.; // [ 0, 1]
        double _SmoothedSpectralFlux    = 0.; // [ 0, 1]

        double _SmoothedBassLevel       = 0.; // [ 0, 1]
        double _SmoothedMidLevel        = 0.; // [ 0, 1]
        double _SmoothedTrebleLevel     = 0.; // [ 0, 1]

        double _SmoothedStereoBalance   = 0.; // [-1, 1]

        double _SmoothedCrestFactor     = 0.; // dB
        double _SmoothedPhaseEnergy     = 0.; // [ 0, 1]

    protected:
        // Musical events:
        // _BeatAverage estimates the normal or local bass level of the current music.
        // A fixed bass threshold would not work reliably because different recordings have very different levels:
        // - one track may have quiet bass,
        // - another may have heavily compressed bass,
        // - another may have a continuous low-frequency drone,
        // - another may contain isolated kick drums.
        // Instead, _BeatAverage follows the recent bass level and provides an adaptive reference.
        double _BeatAverage             = 0.;
        double _BeatEnvelope            = 0.;
        double _BeatPulse               = 0.; // Represents a qualified beat event.
        double _BeatCooldown            = 0.; // In seconds

        // Derived animation controls
        double _BassImpulse             = 0.; // Represents transient visual energy.
        double _BassExpansion           = 0.;
        double _BassTwist               = 0.;

    protected:
        double _PreviousPeak            = 0.;
        double _PreviousRMS             = 0.;

        double _PreviousBassLevel       = 0.;
        double _PreviousMidLevel        = 0.;
        double _PreviousTrebleLevel     = 0.;

        double _PreviousBeatEnvelope    = 0.;

    protected:
        // First-order low-pass filter states.
        double _SubBassState            = 0.;
        double _BassState               = 0.;
        double _MidState                = 0.;
        bool _IsFilterPrimed = false;
    };
}

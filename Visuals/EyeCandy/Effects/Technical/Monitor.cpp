
/** $VER: Monitor.cpp (2026.11.11) **/

#include <pch.h>

#include "Monitor.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Monitor
{
    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return L"Monitor";
    }

    /// <summary>
    /// Clears temporal measurements while retaining device resources.
    /// </summary>
    void effect_t::Reset() noexcept
    {
        effect_processor_t::Reset();

        // Non-standard indicators
        _MonoCompatibility      = 0.;
        _ClippedFramesRatio     = 0.;
        _DCOffset               = 0.;

        _BassDetector           = 0.;
        _BassHitIntensity       = 0.;
        _BassHitEvent           = false;
        _PreviousRMS            = 0.;

        _DrumDetector           = 0.;
        _DrumCooldown           = 0.;
        _DrumHitIntensity       = 0.;
        _DrumHitEvent           = false;

        _TransientRate          = 0.;

        for (auto & Band : _MonitorBands)
        {
            Band.Energy         = 0.;
            Band.SmoothedEnergy = 0.;
            Band.PreviousEnergy = 0.;
        }

        for (auto & Gauge : Gauges)
        {
            Gauge.Value = 0.;
        }

        // Derived measurements.
        _SmoothedPeak           = 0.;
        _SmoothedRMS            = 0.;

        _SmoothedCrestFactor    = 0.;
        _SmoothedStereoWidth    = 0.;
        _SmoothedPhaseEnergy    = 0.;
        _SmoothedSpectralFlux   = 0.;

        _SmoothedBassLevel      = 0.;
        _SmoothedMidLevel       = 0.;
        _SmoothedTrebleLevel    = 0.;

        _SmoothedCorrelation    = 0.;

        // History
        _PeakHistory            = { };
        _RMSHistory             = { };
        _CorrelationHistory     = { };
        _FluxHistory            = { };
        _BassHitHistory         = { };
        _DrumHitHistory         = { };
        _BassLevelHistory       = { };
        _MidLevelHistory        = { };
        _TrebleLevelHistory     = { };

        _HistoryElapsed         = 0.;
    }

    namespace
    {
        constexpr double SignalThreshold = 1e-5; // Approximately -100 dBFS.
    }

    /// <summary>
    /// Selects the first stereo pair. Mono sources use channel zero for both sides.
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept
    {
        const size_t ChannelCount = chunk.get_channel_count();

        const channel_pair_t SelectedChannels{ 0, (ChannelCount > 1) ? 1u : 0u };

        Render(renderTarget, deltaTime, chunk, SelectedChannels);
    }

    /// <summary>
    /// Updates the measurements and renders a calibrated audio monitor.
    /// </summary>
    void effect_t::Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk, const channel_pair_t & selectedChannels) noexcept
    {
        const HRESULT hr = CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return;

        deltaTime = std::clamp(deltaTime, 0., 0.25);

        AnalyzeAudio(chunk, selectedChannels);
        Update(deltaTime);

        renderTarget->Clear(CanvasColor);

        if ((_Width < 2.f) || (_Height < 2.f))
            return;

        const float ShortSide      = std::min(_Width, _Height);
        const float Margin         = std::clamp(ShortSide * 0.018f, 2.f, 14.f);
        const float Gap            = std::clamp(ShortSide * 0.012f, 2.f, 10.f);
        const float ContentWidth   = std::max(1.f, _Width - Margin * 2.f);
        const float ContentHeight  = std::max(1.f, _Height - Margin * 2.f);
        const float AvailableWidth = std::max(1.f, ContentWidth - Gap);

        // Keep a fixed two-column composition at every viewport size. The history panel occupies the full left column; gauges and frequency bands share the right column.
        const float HistoryWidth         = AvailableWidth * 0.46f;
        const float RightWidth           = AvailableWidth - HistoryWidth;
        const float AvailableRightHeight = std::max(1.f, ContentHeight - Gap);
        const float GaugeHeight          = AvailableRightHeight * (2.f / 3.f);
        const float HistogramHeight      = AvailableRightHeight - GaugeHeight;
        const float RightLeft            = Margin + HistoryWidth + Gap;

        const D2D1_RECT_F History    = D2D1::RectF(Margin, Margin, Margin + HistoryWidth, Margin + ContentHeight);
        const D2D1_RECT_F GaugesRect = D2D1::RectF(RightLeft, Margin, RightLeft + RightWidth, Margin + GaugeHeight);
        const D2D1_RECT_F Histogram  = D2D1::RectF(RightLeft, GaugesRect.bottom + Gap, RightLeft + RightWidth, GaugesRect.bottom + Gap + HistogramHeight);

        RenderHistory(renderTarget, History);
        RenderGauges(renderTarget, GaugesRect);
        RenderHistogram(renderTarget, Histogram);

        RenderFrame(renderTarget, BorderColor);
    }

    /// <summary>
    /// Calculates time-domain level, stereo and crest measurements from the selected channels.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels) noexcept
    {
        if (chunk.is_empty())
        {
            effect_processor_t::Reset();

            Reset();

            return;
        }

        const audio_sample * const Frames = chunk.get_data();

        const auto FrameCount   = chunk.get_sample_count();
        const auto ChannelCount = chunk.get_channel_count();

        if ((FrameCount < 16) || (channels[0] >= ChannelCount) || (channels[1] >= ChannelCount))
            return;

        if (_SampleRate != (double) chunk.get_sample_rate())
        {
            _SampleRate = (double) chunk.get_sample_rate();

            CalculateBandEdges(LoFrequency, HiFrequency, _SampleRate);
            CalculateBandLabels();
        }

        _Peak = 0.;

        const bool BassFilterConfigurationChanged = !_BassFilterPrimed || (_BassFilterLeftChannel  != channels[0]) || (_BassFilterRightChannel != channels[1]) || (std::abs(_BassFilterSampleRate - _SampleRate) > 0.5);

        if (BassFilterConfigurationChanged)
        {
            _BassFilterLeftChannel  = channels[0];
            _BassFilterRightChannel = channels[1];
            _BassFilterSampleRate   = _SampleRate;
            _BassFilterPrimed       = false;
        }

        double SumLeftSquares  = 0.;
        double SumRightSquares = 0.;
        double SumCross        = 0.;

        double SumSquares = 0.;

        double SumMidSquares  = 0.;
        double SumSideSquares = 0.;

        double SumLeft  = 0.;
        double SumRight = 0.;

        double SumBassLeftSquares  = 0.;
        double SumBassRightSquares = 0.;
        double SumBassCross        = 0.;

        size_t ClippedFrameCount  = 0;

        // Analyze only the most recent frames of the chunk.
        const auto MaxFrames = FrameCount;

        const double BassCoefficient = 1. - std::exp(-TwoPi * BassCutoff / std::max(1., _SampleRate));

        {
            if (_Samples.size() < MaxFrames)
                _Samples.resize(MaxFrames);

            auto Samples = std::span<double>(_Samples.data(), MaxFrames);

            const audio_sample * Frame = Frames;

            for (size_t i = 0; i < MaxFrames; ++i, Frame += ChannelCount)
            {
                const double Left  = Frame[channels[0]];
                const double Right = Frame[channels[1]];

                // Mid/side decomposition
                const double Mid   = 0.5 * (Left + Right); // Represents common, centered content.
                const double Side  = 0.5 * (Left - Right); // Represents channel differences.

                {
                    SumLeftSquares  += Left  * Left;
                    SumRightSquares += Right * Right;
                    SumCross        += Left  * Right;

                    SumSquares      += 0.5 * ((Left  * Left) + (Right * Right));

                    SumMidSquares   += Mid  * Mid;
                    SumSideSquares  += Side * Side;

                    SumLeft         += Left;
                    SumRight        += Right;
                }

                {
                    // One-pole low-pass filter
                    if (!_BassFilterPrimed)
                    {
                        // Prime from the first actual sample instead of ramping from zero.
                        _BassFilterLeft   = Left;
                        _BassFilterRight  = Right;
                        _BassFilterPrimed = true;
                    }
                    else
                    {
                        _BassFilterLeft  += (Left  - _BassFilterLeft)  * BassCoefficient;
                        _BassFilterRight += (Right - _BassFilterRight) * BassCoefficient;
                    }

                    SumBassLeftSquares  += _BassFilterLeft  * _BassFilterLeft;
                    SumBassRightSquares += _BassFilterRight * _BassFilterRight;
                    SumBassCross        += _BassFilterLeft  * _BassFilterRight;
                }

                _Peak = std::max(_Peak, std::max(std::abs(Left), std::abs(Right)));

                if (std::abs(Left) >= 0.999 || std::abs(Right) >= 0.999)
                    ++ClippedFrameCount;

                // Store the mono sample.
                Samples[i] = Mid;
            }

            AnalyzeSpectrum(Samples);
        }

        const double LeftRMS  = std::sqrt(SumLeftSquares  / (double) MaxFrames);
        const double RightRMS = std::sqrt(SumRightSquares / (double) MaxFrames);

        const double MidRMS   = std::sqrt(SumMidSquares   / (double) MaxFrames);
        const double SideRMS  = std::sqrt(SumSideSquares  / (double) MaxFrames);

        {
            // Peak indicates how close the signal gets to full scale. It is useful for:
            // - Showing short loud events;
            // - Warning about possible clipping;
            // - Driving fast-reacting bars or flashes;
            // - Comparing short peaks with average loudness.
            _Peak = std::clamp(_Peak, 0., 1.);

            // RMS follows the sustained energy of the signal better than peak. In an audio visualizer it provides:
            // - A stable representation of perceived intensity;
            // - A body or fill level that moves less erratically than peak;
            // - A useful baseline for onset and transient detection;
            // - Context for whether a large peak is isolated or part of consistently loud material.
            _RMS  = std::clamp(std::sqrt(SumSquares / (double) MaxFrames), 0., 1.);
        }

        // Calulate the crest factor. It describes the difference between instantaneous peaks and average energy.
        // - High crest factor indicates sharp transients or dynamic material.
        // - Low crest factor indicates dense, compressed, limited, or sustained material.
        // For a visualizer, this helps distinguish two signals that have the same RMS level but very different character.
        // A snare hit, for example, can have a much higher crest factor than a heavily limited synth pad.
        // It can drive visual sharpness, particle bursts, edge intensity, or the contrast between a sustained base animation and transient accents.
        {
            _CrestFactor = (_RMS > Epsilon) ? std::clamp(20. * std::log10(std::max(_Peak / _RMS, 1.)), 0., 30.) : 0.;
        }

        // Calculate the normalized left/right cross-correlation.
        // Correlation gives a visual indication of stereo stability and probable mono behavior:
        // - Positive values generally imply safe mono summing;
        // - Values near zero indicate a wide or decorrelated image;
        // - Sustained negative values warn of cancellation when summed to mono.
        // It is especially useful as a centered horizontal gauge because the sign is as meaningful as the magnitude.
        {
            const double d = std::sqrt(SumLeftSquares * SumRightSquares);

            _Correlation = (d > Epsilon) ? std::clamp(SumCross / d, -1., 1.) : 0.;
        }

        // Calculate the bass correlation.
        // Low frequencies are commonly kept centered because out-of-phase bass:
        // - can cancel badly in mono;
        // - can behave unpredictably on vinyl;
        // - can produce unstable subwoofer reproduction;
        // - may consume headroom without producing a stable perceived bass image.
        // A dedicated bass-correlation meter can reveal problems hidden by otherwise acceptable full-band correlation.
        {
            const double d = std::sqrt(SumBassLeftSquares * SumBassRightSquares);

            _BassCorrelation = (d > Epsilon) ? std::clamp(SumBassCross / d, -1., 1.) : 0.;
        }

        // Calculate the stereo width (0 to 100%). It measures the amount of side content.
        // Unlike correlation, width measures the amount of side content, not whether that content is phase-safe. The two gauges therefore complement each other:
        // - wide with positive correlation can be healthy stereo;
        // -wide with negative correlation can indicate cancellation risk.
        {
            _StereoWidth = (MidRMS + SideRMS) > Epsilon ? std::clamp(SideRMS / (MidRMS + SideRMS), 0., 1.) : 0.;
        }

        // Calculate the phase energy (Energy-oriented diagnostic).
        // It measures how much total mid/side energy resides in the side channel. StereoWidth and PhaseEnergy communicate closely related information.
        // Compared with the stereo-width formula, it is based directly on energy rather than RMS magnitudes.
        // It is useful as:
        // - A compact “stereo difference energy” control signal;
        // - A driver for visual spread or lateral motion;
        // - An indication of passages dominated by ambience, stereo effects, or phase differences.
        {
            const double t = SumMidSquares + SumSideSquares;

            _PhaseEnergy = (t > Epsilon) ? std::clamp(SumSideSquares / t, 0., 1.) : 0.;
        }

        // Calculate the left/right channel balance (in dB). Negative values indicate left dominance; positive values indicate right dominance.
        // Balance identifies:
        // - Asymmetric mixes;
        // - Panning changes;
        // - Damaged or missing channels;
        // - Visual movement associated with left/right emphasis.
        // A bipolar horizontal meter is particularly appropriate because zero is the desired center reference.
        {
            _ChannelBalance = std::clamp(20. * std::log10(std::max(RightRMS, 0.000001) / std::max(LeftRMS, 0.000001)), -18., 18.);
        }

        // Calculate mono compatibility (Non-standard indicator).
        // The mid channel is what remains after equal left/right summing, apart from scaling.
        // A low result therefore indicates that much of the signal exists as channel difference and may weaken in mono.
        // This is useful because correlation can fluctuate rapidly, while the mid-energy ratio offers a direct estimate of how much common content survives.
        {
            _MonoCompatibility = (_RMS > Epsilon) ? std::clamp(MidRMS / _RMS, 0., 1.) : 0.;
        }

        // Calculate the ratio of clipped frames (Non-standard indicator).
        {
            _ClippedFramesRatio = std::clamp((double) ClippedFrameCount  / (double) MaxFrames, 0., 1.);
        }

        // Calculate the DC offset.
        // Normal audio should oscillate around zero. A sustained nonzero mean can indicate:
        // - Biased capture hardware;
        // - Defective processing;
        // - Asymmetric waveform shaping;
        // - Problematic synthesized signals.
        // DC itself is not musically interesting, but it is a useful health indicator. The code flags absolute values above 1 mFS with a caution color.
        {
            _DCOffset = std::clamp(500. * ((SumLeft + SumRight) / (double) MaxFrames), -10., 10.);
        }

        _HasAudio = (_Peak > SignalThreshold) || (_RMS > SignalThreshold);
    }

    /// <summary>
    /// Analyzes the spectrum of the supplied mono samples.
    /// </summary>
    void effect_t::AnalyzeSpectrum(const std::span<double> & samples) noexcept
    {
        if ((samples.size() < 16) || (_SampleRate <= 0.))
            return;

        const size_t SampleCount = samples.size();

        // Precalulate the Hann window function.
        if (_Window.size() != SampleCount)
        {
            _Window.resize(SampleCount);

            double WindowSum = 0.;

            for (size_t i = 0; i < SampleCount; ++i)
            {
                _Window[i] = 0.5 - 0.5 * std::cos(TwoPi * (double) i / (double) (SampleCount - 1));

                WindowSum += _Window[i];
            }

            // WindowGain is the coherent amplitude gain. Dividing by it restores the amplitude of a sinusoid located exactly at a DFT bin frequency.
            _WindowGain = WindowSum / (double) SampleCount;

            if (_WindowGain <= Epsilon)
                return;
        }

        // Each entry accumulates RMS power assigned to that monitor band.
        // BandWeights records whether a band received any contribution. It is not used to normalize the power because the desired result is integrated band power, not average spectral density per bin.
        std::array<double, MonitorBandCount> BandPowers {};
        std::array<double, MonitorBandCount> BandWeights {};

        const double FrequencyResolution = _SampleRate / (double) SampleCount;
        const size_t NyquistBinIndex = SampleCount / 2;

        // Include every bin whose represented frequency interval can overlap the configured maximum frequency. Bin i represents approximately: [(i - 0.5) * resolution, (i + 0.5) * resolution]
        const size_t LastRelevantBin = std::min(NyquistBinIndex, (size_t) (std::ceil((HiFrequency / FrequencyResolution) + 0.5)));

        for (size_t BinIndex = 1; BinIndex <= LastRelevantBin; ++BinIndex)
        {
            double re = 0.;
            double im = 0.;

            {
                double reOscillator = 1.;
                double imOscillator = 0.;

                const double StepAngle = -TwoPi * (double) BinIndex / (double) SampleCount;

                const double reStep = std::cos(StepAngle);
                const double imStep = std::sin(StepAngle);

                for (size_t SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
                {
                    const double Value = samples[SampleIndex] * _Window[SampleIndex];

                    re += Value * reOscillator;
                    im += Value * imOscillator;

                    const double reNext = (reOscillator * reStep) - (imOscillator * imStep);

                    imOscillator = (reOscillator * imStep) + (imOscillator * reStep);
                    reOscillator = reNext;
                }
            }

            // Convert the one-sided DFT result to peak sinusoidal amplitude. DC is skipped because BinIndex starts at one.
            double AmplitudeScale = 2.;

            // The Nyquist component has no negative-frequency partner and therefore must not receive the normal one-sided factor of two.
            if (((SampleCount % 2) == 0) && (BinIndex == NyquistBinIndex))
                    AmplitudeScale = 1.;

            const double Magnitude = AmplitudeScale * std::sqrt(re * re + im * im) / ((double) SampleCount * _WindowGain);

            // Magnitude is peak amplitude for ordinary positive-frequency bins. Convert squared peak amplitude to RMS power.
            // The Nyquist bin represents a real alternating sequence. Its amplitude is already an RMS-equivalent discrete-time value.
            const double BinPower = (BinIndex == NyquistBinIndex) ? (Magnitude * Magnitude) : 0.5 * (Magnitude * Magnitude);

            const double CenterFrequency = (double) BinIndex * FrequencyResolution;

            const double BinLowerFrequency = std::max(              0. , CenterFrequency - (0.5 * FrequencyResolution));
            const double BinUpperFrequency = std::min(_SampleRate * 0.5, CenterFrequency + (0.5 * FrequencyResolution));

            const double BinWidth = BinUpperFrequency - BinLowerFrequency;

            if (BinWidth <= 0.)
                continue;

            // Distribute the bin power over all monitor bands whose frequency intervals overlap this bin interval. This prevents narrow low-frequency bands from remaining empty when the DFT resolution is wider than a band.
            for (size_t BandIndex = 0; BandIndex < MonitorBandCount; ++BandIndex)
            {
                const double OverlapLowerFrequency = std::max(BinLowerFrequency, _BandEdges[BandIndex]);
                const double OverlapUpperFrequency = std::min(BinUpperFrequency, _BandEdges[BandIndex + 1]);

                if (OverlapUpperFrequency <= OverlapLowerFrequency)
                    continue;

                const double OverlapWidth = OverlapUpperFrequency - OverlapLowerFrequency;
                const double Weight = std::clamp(OverlapWidth / BinWidth, 0., 1.);

                BandPowers [BandIndex] += BinPower * Weight;
                BandWeights[BandIndex] += Weight;
            }
        }

        // Convert integrated band power to RMS amplitude and calculate positive spectral flux.
        {
            double PositiveDifference = 0.;
            double ReferenceEnergy = Epsilon;

            for (size_t BandIndex = 0; BandIndex < MonitorBandCount; ++BandIndex)
            {
                const double Energy = (BandWeights[BandIndex] > 0.) ? std::sqrt(std::max(0., BandPowers[BandIndex])) : 0.;

                _MonitorBands[BandIndex].Energy = std::clamp(Energy, 0., 1.);

                PositiveDifference += std::max(0., _MonitorBands[BandIndex].Energy - _MonitorBands[BandIndex].PreviousEnergy);
                ReferenceEnergy += _MonitorBands[BandIndex].PreviousEnergy;

                _MonitorBands[BandIndex].PreviousEnergy = _MonitorBands[BandIndex].Energy;
            }

            // Calculate the spectral flux.
            // Spectral flux responds when new spectral energy appears. It is particularly useful for detecting:
            // - Note attacks
            // - Percussion
            // - Rhythmic changes
            // - Sudden timbral changes
            // Positive-only flux is a good visualizer choice because attacks are generally more useful for event animation than decays.
            {
                _SpectralFlux = std::clamp(PositiveDifference / ReferenceEnergy, 0., 1.);
            }
        }

        {
            constexpr double PowerFloor = 1e-12;

            double TotalPower = 0.;
            double LogPowerSum = 0.;

            // Calculate the spectral centroid. It is a useful measure of perceptual brightness.
            // - Lower centroid suggests dark, bass-heavy, or muffled content.
            // - Higher centroid suggests bright, sharp, or treble-heavy content.
            // It can drive visual color temperature, vertical position, glow, or detail density more reliably than using treble level alone.
            {
                double WeightedFrequency = 0.;

                for (size_t BandIndex = 0; BandIndex < MonitorBandCount; ++BandIndex)
                {
                    const double CenterFrequency = std::sqrt(_BandEdges[BandIndex] * _BandEdges[BandIndex + 1]);
                    const double Energy = _MonitorBands[BandIndex].Energy;

                    const double ActualPower = Energy * Energy;

                    TotalPower += ActualPower;

                    WeightedFrequency += CenterFrequency * ActualPower;

                    // Apply the floor only for the logarithm. Do not add the artificial floor to TotalPower or the centroid weighting.
                    LogPowerSum += std::log(std::max(ActualPower, PowerFloor));
                }

                _SpectralCentroid = (TotalPower > PowerFloor) ? std::clamp(WeightedFrequency / TotalPower, LoFrequency, HiFrequency) : LoFrequency;
            }

            // Calculate the spectral flatness.
            // - Near 0: energy is concentrated in a few bands, indicating tonal or harmonic content.
            // - near 1: energy is distributed relatively evenly, indicating noise-like content.
            // Flatness differentiates signals that may otherwise have similar level and brightness:
            // - A flute note is relatively tonal;
            // - White noise is highly flat;
            // - Cymbals and distorted textures tend to be flatter than clean pitched instruments.
            // A visualizer can use this to move between coherent shapes and diffuse particles, or between smooth and noisy textures.
            {
                const double GeometricMean  = std::exp(LogPowerSum / (double) MonitorBandCount);
                const double ArithmeticMean = TotalPower           / (double) MonitorBandCount;

                _SpectralFlatness = (ArithmeticMean > PowerFloor) ? std::clamp(GeometricMean / ArithmeticMean, 0., 1.) : 0.;
            }
        }

        // Combine band energy using RMS aggregation. Arithmetic averaging suppresses narrow-band content and makes the regional histories appear incorrectly flat.
        // These broad regions are ideal visualizer control signals:
        // - Bass can drive scale, pulse, zoom, or lower-screen motion.
        // - Mid can drive body, density, or primary shape deformation.
        // - Treble can drive particles, sparkle, edge detail, or rapid movement.
        // RMS aggregation is a good decision because a strong tone in one band remains visually significant instead of being diluted by quiet neighboring bands.
        const auto CalculateRegionLevel = [&](size_t firstBand, size_t endBand) noexcept
        {
            firstBand = std::min(firstBand, _MonitorBands.size());
            endBand   = std::min(endBand,   _MonitorBands.size());

            if (firstBand >= endBand)
                return 0.;

            double RegionPower = 0.;

            for (size_t BandIndex = firstBand; BandIndex < endBand; ++BandIndex)
            {
                const double Energy = _MonitorBands[BandIndex].Energy;

                RegionPower += Energy * Energy;
            }

            return std::clamp(std::sqrt(RegionPower), 0., 1.);
        };

        constexpr size_t BassEnd = 6;
        constexpr size_t MidEnd = 12;

        static_assert(BassEnd < MidEnd);
        static_assert(MidEnd < MonitorBandCount);

        _BassLevel   = CalculateRegionLevel(0, BassEnd);
        _MidLevel    = CalculateRegionLevel(BassEnd, MidEnd);
        _TrebleLevel = CalculateRegionLevel(MidEnd, MonitorBandCount);
    }

   /// <summary>
    /// Applies asymmetric ballistics: fast attack and slower release for readable instrumentation.
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        _AnimationTime += deltaTime;

        if (!_HasAudio)
        {
            effect_processor_t::Reset();

            Reset();

            RecordHistory(deltaTime);

            return;
        }

        // The analyzed values are not rendered directly. Most receive fast attack and slower release smoothing.
        // It makes the display responsive without becoming visually noisy. Fast attack preserves musical timing, while slower release gives users enough time to perceive the result.
        {
            _SmoothedPeak         = ExponentialSmoothing(_SmoothedPeak,         _Peak,         deltaTime, (_Peak > _SmoothedPeak) ? 24. : 2.4);
            _SmoothedRMS          = ExponentialSmoothing(_SmoothedRMS,          _RMS,          deltaTime, (_RMS  > _SmoothedRMS)  ? 14. : 2.2);

            _SmoothedCrestFactor  = ExponentialSmoothing(_SmoothedCrestFactor,  _CrestFactor,  deltaTime, 4.0);
            _SmoothedStereoWidth  = ExponentialSmoothing(_SmoothedStereoWidth,  _StereoWidth,  deltaTime, 5.0);

            _SmoothedCorrelation  = ExponentialSmoothing(_SmoothedCorrelation,  _Correlation,  deltaTime, 5.0);
            _SmoothedSpectralFlux = ExponentialSmoothing(_SmoothedSpectralFlux, _SpectralFlux, deltaTime, (_SpectralFlux > _SmoothedSpectralFlux) ? 12. : 3.0);
            _SmoothedPhaseEnergy  = ExponentialSmoothing(_SmoothedPhaseEnergy,  _PhaseEnergy,  deltaTime, (_PhaseEnergy  > _SmoothedPhaseEnergy)  ? 12. : 3.0);

            for (auto & Band : _MonitorBands)
                Band.SmoothedEnergy = ExponentialSmoothing(Band.SmoothedEnergy, Band.Energy, deltaTime, (Band.Energy > Band.SmoothedEnergy) ? 15. : 3.0);

            _SmoothedBassLevel    = ExponentialSmoothing(_SmoothedBassLevel,    _BassLevel,    deltaTime, 5.0);
            _SmoothedMidLevel     = ExponentialSmoothing(_SmoothedMidLevel,     _MidLevel,     deltaTime, 5.0);
            _SmoothedTrebleLevel  = ExponentialSmoothing(_SmoothedTrebleLevel,  _TrebleLevel,  deltaTime, 5.0);
        }

        {
            // Bass events use a slow adaptive floor and the positive slope of the low-frequency envelope.

            _BeatAverage  = ExponentialSmoothing(_BeatAverage,  _BassLevel, deltaTime, 1.25);
            _BeatEnvelope = ExponentialSmoothing(_BeatEnvelope, _BassLevel, deltaTime, _BassLevel > _BeatEnvelope ? 24. : 5.0);

            _BeatCooldown = std::max(0., _BeatCooldown - deltaTime);

            const double BassRise       = std::max(0., _BeatEnvelope - _PreviousBeatEnvelope);
            const double BassThreshold  = 0.006 + (_BeatAverage * 0.10);
            const double BassConfidence = std::clamp(((BassRise - BassThreshold) / std::max(0.01, BassThreshold * 2.5)) * _BassDetectorSensitivity, 0., 1.);

            _PreviousBeatEnvelope = _BeatEnvelope;

            // The bass detector combines:
            // - A fast attack and slower release envelope
            // - The positive rise of that envelope
            // - A sensitivity multiplier [0.25, 4]
            // - A 120ms cooldown
            // An event requires confidence of at least 0.35 and current bass energy of at least 105% of its adaptive average.
            // An adaptive threshold works across quiet and loud tracks better than one fixed amplitude threshold.
            // The cooldown prevents one kick from producing several visual triggers. This is appropriate for beat pulses, zoom effects, background flashes, and bass-reactive geometry.
            {
                _BassDetector = std::max(0., _BassDetector - deltaTime * 4.0);

                if ((_BeatCooldown <= 0.) && (BassConfidence >= 0.35) && (_BassLevel >= _BeatAverage * 1.05))
                {
                    _BassDetector     = 1.;
                    _BassHitIntensity = std::max(_BassHitIntensity, BassConfidence);
                    _BassHitEvent     = true;

                    _BeatPulse        = 1.;
                    _BeatCooldown     = 0.120; // In seconds
                }

                _BassDetector = std::max(_BassDetector, BassConfidence);
                _BeatPulse    = std::max(0., _BeatPulse - (deltaTime * 5.0));
            }

            // Drum events combine broadband spectral change, level onset and bass evidence.

            // The drum detector combines:
            // - Spectral flux
            // - Positive RMS rise
            // - Bass confidence
            // It is adjusted by sensitivity, requires confidence of at least 0.42 and peak amplitude of at least 0.02, and applies an 85 ms cooldown.
            // No single measurement identifies every drum hit:
            // - Spectral flux catches broadband change;
            // - RMS rise catches level onset;
            // - Bass evidence helps with kick drums.
            // The combination is more robust across kicks, snares, toms, and other percussive events than a bass-only beat detector.
            // It remains a heuristic onset detector rather than a tempo tracker. It identifies probable events but does not infer BPM, meter, or beat position.
            {
                const double RMSRise = std::max(0., _RMS - _PreviousRMS);
                const double DrumConfidence = std::clamp(((_SpectralFlux * 0.62) + (RMSRise * 8.0) + (BassConfidence * 0.28)) * _DrumDetectorSensitivity, 0., 1.);

                _DrumCooldown = std::max(0., _DrumCooldown - deltaTime);
                _DrumDetector = std::max(0., _DrumDetector - deltaTime * 5.5);

                if ((_DrumCooldown <= 0.) && (DrumConfidence >= 0.42) && (_Peak >= 0.02))
                {
                    _DrumDetector     = 1.;
                    _DrumHitIntensity = std::max(_DrumHitIntensity, DrumConfidence);
                    _DrumHitEvent     = true;
                    _DrumCooldown     = 0.085;
                }

                _DrumDetector = std::max(_DrumDetector, DrumConfidence);
            }

            _PreviousRMS = _RMS;
        }

        // Calculate transient rate.
        // Transient rate represents longer-term rhythmic density rather than a single event:
        // - Low values indicate sparse or sustained material;
        // - High values indicate busy percussion or rapid articulation.
        // This is useful for controlling animation complexity, particle count, camera movement, or overall visual activity.
        {
            const double TransientDecay = std::exp(-deltaTime / 3.0);

            _TransientRate *= TransientDecay;

            if (_DrumHitEvent)
                _TransientRate += (1. - TransientDecay) / std::max(deltaTime, 1. / 240.);

            _TransientRate = std::clamp(_TransientRate, 0., 12.);
        }

        // Update the gauges.
        {
            Gauges[ 0].Value = AmplitudeToDecibels(_SmoothedPeak);
            Gauges[ 1].Value = AmplitudeToDecibels(_SmoothedRMS);
            Gauges[ 2].Value = _SmoothedCrestFactor;
            Gauges[ 3].Value = _SmoothedStereoWidth * 100.;
            Gauges[ 4].Value = _SmoothedPhaseEnergy;
            Gauges[ 5].Value = _SmoothedSpectralFlux;
            Gauges[ 6].Value = _BassDetector * 100.;
            Gauges[ 6].Color = (_BassDetector >= 0.35) ? BassEventColor : DataColor;
            Gauges[ 7].Value = _DrumDetector * 100.;
            Gauges[ 7].Color = (_DrumDetector >= 0.42) ? DrumEventColor : DataColor;
            Gauges[ 8].Value = _SpectralCentroid;
            Gauges[ 9].Value = _SpectralFlatness;
            Gauges[10].Value = _MonoCompatibility * 100.;
            Gauges[11].Value = _ClippedFramesRatio * 100.;
            Gauges[11].Color = (_ClippedFramesRatio > 0.) ? CriticalColor : DataColor;
            Gauges[12].Value = _DCOffset;
            Gauges[12].Color = (std::abs(_DCOffset) > 1.) ? CautionColor : DataColor;
            Gauges[13].Value = _BassCorrelation;
            Gauges[13].Color = (_BassCorrelation < 0.) ? CriticalColor : DataColor;
            Gauges[14].Value = _TransientRate;

            HorizontalGauges[ 0].Value = _ChannelBalance;
            HorizontalGauges[ 1].Value = _SmoothedCorrelation;
            HorizontalGauges[ 1].Color = (_SmoothedCorrelation < 0.) ? CriticalColor : DataColor;

            constexpr double PeakHoldDuration = 1.5; // In seconds
            constexpr double PeakHoldRelease = 0.18;

            for (auto & Gauge : Gauges)
            {
                if (Gauge.Value >= Gauge.PeakHoldValue)
                {
                    Gauge.PeakHoldValue   = Gauge.Value;
                    Gauge.PeakHoldElapsed = 0.;
                }
                else
                {
                    Gauge.PeakHoldElapsed += deltaTime;

                    if (Gauge.PeakHoldElapsed >= PeakHoldDuration)
                        Gauge.PeakHoldValue = std::max(Gauge.Value, Gauge.PeakHoldValue - (PeakHoldRelease * deltaTime) * std::max(1., std::abs(Gauge.PeakHoldValue - Gauge.Minimum)));
                }
            }
        }

        RecordHistory(deltaTime);
    }

    /// <summary>
    /// Samples histories at a stable 30 Hz, independent of the rendering frame rate.
    /// </summary>
    void effect_t::RecordHistory(double deltaTime) noexcept
    {
        _HistoryElapsed += deltaTime;

        auto Push = [](history_t & history, double value) noexcept
        {
            history.Values[history.Cursor] = value;
            history.Cursor = (history.Cursor + 1) % HistoryCount;
            history.Size = std::min(history.Size + 1, HistoryCount);
        };

        constexpr double HistoryInterval = 1. / 30.;

        while (_HistoryElapsed >= HistoryInterval)
        {
            _HistoryElapsed -= HistoryInterval;

            Push(_PeakHistory,        AmplitudeToDecibels(_SmoothedPeak));
            Push(_RMSHistory,         AmplitudeToDecibels(_SmoothedRMS));

            Push(_CorrelationHistory, _SmoothedCorrelation);
            Push(_FluxHistory,        _SmoothedSpectralFlux);

            Push(_BassHitHistory,     _BassHitEvent ? _BassHitIntensity : 0.);
            Push(_DrumHitHistory,     _DrumHitEvent ? _DrumHitIntensity : 0.);

            // The regional values are RMS amplitudes, so 20 log10 provides a calibrated dBFS history with enough visual range for normal programme material.
            Push(_BassLevelHistory,   AmplitudeToDecibels(_SmoothedBassLevel));
            Push(_MidLevelHistory,    AmplitudeToDecibels(_SmoothedMidLevel));
            Push(_TrebleLevelHistory, AmplitudeToDecibels(_SmoothedTrebleLevel));

            _BassHitIntensity = 0.;
            _BassHitEvent = false;

            _DrumHitIntensity = 0.;
            _DrumHitEvent = false;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderGauges(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept
    {
        const float Width  = bounds.right  - bounds.left;
        const float Height = bounds.bottom - bounds.top;

        if ((Width < 6.f) || (Height < 20.f))
            return;

        DrawPanel(renderTarget, bounds);

        const float Padding = std::min(12.f, std::max(2.f, Width * 0.012f));
        const float Gap     = std::min( 8.f, std::max(1.f, Width * 0.006f));

        const float HorizontalAreaHeight = std::max(42.f, Height * 0.22f);
        const float VerticalAreaHeight   = std::max( 1.f, Height - Padding * 2.f - HorizontalAreaHeight - Gap);

        constexpr size_t ColumnCount = 8;
        constexpr size_t RowCount    = 2;

        const float AvailableWidth  = std::max(1.f, Width - Padding * 2.f - Gap * (float)(ColumnCount - 1));
        const float AvailableHeight = std::max(1.f, VerticalAreaHeight - Gap * (float)(RowCount - 1));

        {
            const float CellWidth  = AvailableWidth  / (float) ColumnCount;
            const float CellHeight = AvailableHeight / (float) RowCount;

            for (size_t i = 0; i < Gauges.size(); ++i)
            {
                const size_t Column = i % ColumnCount;
                const size_t Row    = i / ColumnCount;

                const float Left = bounds.left + Padding + (float) Column * (CellWidth  + Gap);
                const float Top  = bounds.top  + Padding + (float) Row    * (CellHeight + Gap);

                RenderGauge(renderTarget, D2D1::RectF(Left, Top, Left + CellWidth, Top + CellHeight), Gauges[i]);
            }
        }

        {
            const float HorizontalTop         = bounds.bottom - Padding - HorizontalAreaHeight;
            const float HorizontalGaugeHeight = std::max(1.f, (HorizontalAreaHeight - Gap) * 0.5f);

            for (size_t i = 0; i < HorizontalGauges.size(); ++i)
            {
                const float Top = HorizontalTop + ((float) i * (HorizontalGaugeHeight + Gap));

                RenderHorizontalGauge(renderTarget, D2D1::RectF(bounds.left + Padding, Top, bounds.right - Padding, Top + HorizontalGaugeHeight), HorizontalGauges[i]);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderGauge(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const gauge_t & gauge) noexcept
    {
        const float Width  = bounds.right  - bounds.left;
        const float Height = bounds.bottom - bounds.top;

        if ((Width < 2.f) || (Height < 10.f))
            return;

        SetBrush(InstrumentColor);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(bounds, 5.f, 5.f), _Brush.Get());

        const double Normalized = std::clamp((gauge.Value - gauge.Minimum) / (gauge.Maximum - gauge.Minimum), 0., 1.);
        const float TrackWidth  = std::max(2.f, std::min(24.f, Width * 0.22f));
        const float LabelHeight = std::min(24.f, Height * 0.18f);
        const float ValueHeight = std::min(27.f, Height * 0.20f);
        const float TrackTop    = bounds.top + LabelHeight + 3.f;
        const float TrackBottom = std::max(TrackTop + 1.f, bounds.bottom - ValueHeight - 3.f);
        const float TrackLeft   = bounds.left + (Width - TrackWidth) * 0.5f;

        const D2D1_RECT_F Track = D2D1::RectF(TrackLeft, TrackTop, TrackLeft + TrackWidth, TrackBottom);

        const float FillTop = Lerp(Track.bottom, Track.top, (float)Normalized);

        DrawText(renderTarget, gauge.Label, D2D1::RectF(bounds.left + 5.f, bounds.top + 5.f, bounds.right - 5.f, bounds.top + LabelHeight), _SmallFormat.Get(), LabelColor);

        SetBrush(TrackColor);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Track, TrackWidth * 0.35f, TrackWidth * 0.35f), _Brush.Get());

        const D2D1_RECT_F Fill = D2D1::RectF(Track.left, FillTop, Track.right, Track.bottom);

        SetBrush(gauge.Color);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Fill, TrackWidth * 0.35f, TrackWidth * 0.35f), _Brush.Get());

        const double PeakNormalized = std::clamp((gauge.PeakHoldValue - gauge.Minimum) / (gauge.Maximum - gauge.Minimum), 0., 1.);
        const float PeakY = Lerp(Track.bottom, Track.top, (float)PeakNormalized);

        SetBrush(D2D1::ColorF(1.f, 1.f, 1.f, 0.95f));

        renderTarget->DrawLine(D2D1::Point2F(Track.left - 3.f, PeakY), D2D1::Point2F(Track.right + 3.f, PeakY), _Brush.Get(), 2.f);

        // Major ticks make the scale readable without turning the compact meter into a chart.
        for (size_t i = 0; i <= 4; ++i)
        {
            const float Amount = (float) i / 4.f;

            const float Y          = Lerp(Track.bottom, Track.top, Amount);
            const float TickLength = (i == 0 || i == 4) ? 8.f : 5.f;

            SetBrush(MutedTextColor);

            renderTarget->DrawLine(D2D1::Point2F(Track.left - TickLength, Y), D2D1::Point2F(Track.left - 2.f,         Y), _Brush.Get(), 1.f);
            renderTarget->DrawLine(D2D1::Point2F(Track.right + 2.f,       Y), D2D1::Point2F(Track.right + TickLength, Y), _Brush.Get(), 1.f);
        }

        const std::wstring Value = gauge.Decimals == 2 ? std::format(L"{:.2f} {}", gauge.Value, gauge.Unit) : std::format(L"{:.1f} {}", gauge.Value, gauge.Unit);

        DrawText(renderTarget, Value, D2D1::RectF(bounds.left + 3.f, bounds.bottom - ValueHeight, bounds.right - 3.f, bounds.bottom - 2.f), _ValueFormat.Get(), White);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderHorizontalGauge(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const gauge_t & gauge) noexcept
    {
        const float Width  = bounds.right - bounds.left;
        const float Height = bounds.bottom - bounds.top;

        if ((Width < 20.f) || (Height < 12.f))
            return;

        SetBrush(InstrumentColor);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(bounds, 5.f, 5.f), _Brush.Get());

        const float LabelWidth  = std::min(118.f, Width * 0.24f);
        const float ValueWidth  = std::min(94.f, Width * 0.20f);
        const float TrackLeft   = bounds.left + LabelWidth + 8.f;
        const float TrackRight  = std::max(TrackLeft + 1.f, bounds.right - ValueWidth - 8.f);
        const float TrackHeight = std::max(4.f, std::min(14.f, Height * 0.28f));
        const float TrackTop    = bounds.top + (Height - TrackHeight) * 0.5f;

        const D2D1_RECT_F Track = D2D1::RectF(TrackLeft, TrackTop, TrackRight, TrackTop + TrackHeight);

        const double Normalized     = std::clamp((gauge.Value         - gauge.Minimum) / (gauge.Maximum - gauge.Minimum), 0., 1.);
        const double PeakNormalized = std::clamp((gauge.PeakHoldValue - gauge.Minimum) / (gauge.Maximum - gauge.Minimum), 0., 1.);

        const float ValueX = Lerp(Track.left, Track.right, (float) Normalized);
        const float PeakX  = Lerp(Track.left, Track.right, (float) PeakNormalized);

        DrawText(renderTarget, gauge.Label, D2D1::RectF(bounds.left + 5.f, bounds.top + 2.f, bounds.left + LabelWidth, bounds.bottom - 2.f), _SmallFormat.Get(), LabelColor);

        SetBrush(TrackColor);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Track, TrackHeight * 0.35f, TrackHeight * 0.35f), _Brush.Get());

        const bool Bipolar = (gauge.Minimum < 0.) && (gauge.Maximum > 0.);
        const float ZeroX = Bipolar ? Lerp(Track.left, Track.right, (float)((0. - gauge.Minimum) / (gauge.Maximum - gauge.Minimum))) : Track.left;

        const D2D1_RECT_F Fill = D2D1::RectF(std::min(ZeroX, ValueX), Track.top, std::max(ZeroX, ValueX), Track.bottom);

        SetBrush(gauge.Color);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(Fill, TrackHeight * 0.35f, TrackHeight * 0.35f), _Brush.Get());

        SetBrush(LabelColor);

        renderTarget->DrawLine(D2D1::Point2F(ZeroX, Track.top - 3.f), D2D1::Point2F(ZeroX, Track.bottom + 3.f), _Brush.Get(), 1.f);

        SetBrush(White);

        renderTarget->DrawLine(D2D1::Point2F(PeakX, Track.top - 3.f), D2D1::Point2F(PeakX, Track.bottom + 3.f), _Brush.Get(), 2.f);

        for (size_t i = 0; i <= 4; ++i)
        {
            const float X = Lerp(Track.left, Track.right, (float) i / 4.f);

            SetBrush(MutedTextColor);

            renderTarget->DrawLine(D2D1::Point2F(X, Track.bottom + 2.f), D2D1::Point2F(X, Track.bottom + 5.f), _Brush.Get(), 1.f);
        }

        const std::wstring Value = gauge.Decimals == 2 ? std::format(L"{:.2f} {}", gauge.Value, gauge.Unit) : std::format(L"{:.1f} {}", gauge.Value, gauge.Unit);

        DrawText(renderTarget, Value, D2D1::RectF(bounds.right - ValueWidth, bounds.top + 2.f, bounds.right - 4.f, bounds.bottom - 2.f), _ValueFormat.Get(), White);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderHistogram(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept
    {
        const float Width  = bounds.right - bounds.left;
        const float Height = bounds.bottom - bounds.top;

        if ((Width < 48.f) || (Height < 48.f))
            return;

        DrawPanel(renderTarget, bounds);

        DrawText(renderTarget, L"FREQUENCY BANDS  |  dBFS", D2D1::RectF(bounds.left + 12.f, bounds.top + 7.f, bounds.right - 12.f, bounds.top + 27.f), _LabelFormat.Get(), LabelColor);

        const float Left       = bounds.left + 36.f;
        const float Top        = bounds.top + 32.f;
        const float Right      = bounds.right - 10.f;
        const float Bottom     = bounds.bottom - 25.f;
        const float PlotHeight = std::max(1.f, Bottom - Top);
        const float BarSlot    = (Right - Left) / (float) MonitorBandCount;
        const float BarWidth   = BarSlot * 0.72f;

        for (int db = -60; db <= 0; db += 12)
        {
            const float Amount = (float)((db - MinDecibels) / (MaxDecibels - MinDecibels));

            const float Y = Bottom - (Amount * PlotHeight);

            SetBrush(GridColor);

            renderTarget->DrawLine(D2D1::Point2F(Left, Y), D2D1::Point2F(Right, Y), _Brush.Get(), 1.f);

            DrawText(renderTarget, std::format(L"{}", db), D2D1::RectF(bounds.left + 4.f, Y - 8.f, Left - 4.f, Y + 9.f), _SmallFormat.Get(), MutedTextColor);
        }

        for (size_t i = 0; i < MonitorBandCount; ++i)
        {
            const double Decibels = AmplitudeToDecibels(_MonitorBands[i].SmoothedEnergy);

            const float Amount = (float) std::clamp((Decibels - MinDecibels) / (MaxDecibels - MinDecibels), 0., 1.);

            const float x = Left + ((float) i * BarSlot) + (BarSlot - BarWidth) * 0.5f;

            const D2D1_RECT_F Bar = D2D1::RectF(x, Bottom - (Amount * PlotHeight), x + BarWidth, Bottom);
            const D2D1_COLOR_F BarColor = (Decibels >= -1.) ? CriticalColor : (Decibels >= -6.) ? CautionColor : DataColor;

            SetBrush(BarColor);

            renderTarget->FillRectangle(Bar, _Brush.Get());

            DrawText(renderTarget, _BandLabels[i], D2D1::RectF(x - BarSlot * 0.15f, Bottom + 3.f, x + BarWidth + BarSlot * 0.15f, bounds.bottom - 3.f), _SmallFormat.Get(), LabelColor);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderHistory(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept
    {
        const float Width = bounds.right - bounds.left;
        const float Height = bounds.bottom - bounds.top;

        if ((Width < 48.f) || (Height < 80.f))
            return;

        DrawPanel(renderTarget, bounds);

        const float Padding = std::min(12.f, std::max(3.f, std::min(Width, Height) * 0.018f));
        const float Gap     = std::min(8.f, std::max(2.f, std::min(Width, Height) * 0.012f));

        const size_t Columns = 1;
        const size_t Rows = 8;

        const float GraphWidth  = std::max(1.f, (Width - Padding * 2.f - Gap * (float)(Columns - 1u)) / (float)Columns);
        const float GraphHeight = std::max(1.f, (Height - Padding * 2.f - Gap * (float)(Rows - 1u)) / (float)Rows);

        for (size_t i = 0; i < 8; ++i)
        {
            const size_t Column = i % Columns;
            const size_t Row    = i / Columns;

            const float Left = bounds.left + Padding + (float)Column * (GraphWidth + Gap);
            const float Top  = bounds.top + Padding + (float)Row * (GraphHeight + Gap);

            const D2D1_RECT_F Graph = D2D1::RectF(Left, Top, Left + GraphWidth, Top + GraphHeight);

            switch (i)
            {
                default:

                case 0: RenderHistoryGraph(renderTarget, Graph, L"Peak (dBFS)",             _PeakHistory, -60., 0., DataColor); break;
                case 1: RenderHistoryGraph(renderTarget, Graph, L"RMS  (dBFS)",             _RMSHistory, -60., 0., DataColor); break;

                case 2: RenderHistoryGraph(renderTarget, Graph, L"Spectral Flux",           _FluxHistory, 0., 1., DataColor); break;

                case 3: RenderHistoryGraph(renderTarget, Graph, L"Bass Hits Intensity",     _BassHitHistory, 0., 1., BassEventColor, hit_type_t::Bass); break;
                case 4: RenderHistoryGraph(renderTarget, Graph, L"Drum Hits Intensity",     _DrumHitHistory, 0., 1., DrumEventColor, hit_type_t::Drum); break;

                case 5: RenderHistoryGraph(renderTarget, Graph, L"Bass Energy (dBFS)",      _BassLevelHistory, -60., 0., DataColor); break;
                case 6: RenderHistoryGraph(renderTarget, Graph, L"Mid Energy (dBFS)",       _MidLevelHistory, -60., 0., DataColor); break;
                case 7: RenderHistoryGraph(renderTarget, Graph, L"Treble Energy (dBFS)",    _TrebleLevelHistory, -60., 0., DataColor); break;
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderHistoryGraph(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const wchar_t * label, const history_t & history, double minimum, double maximum, const D2D1_COLOR_F & color, hit_type_t hitType) noexcept
    {
        if (((bounds.right - bounds.left) < 12.f) || ((bounds.bottom - bounds.top) < 18.f))
            return;

        SetBrush(InstrumentColor);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(bounds, 4.f, 4.f), _Brush.Get());

        DrawText(renderTarget, label, D2D1::RectF(bounds.left + 8.f, bounds.top + 4.f, bounds.right - 8.f, bounds.top + 23.f), _SmallFormat.Get(), LabelColor);

        const D2D1_RECT_F Plot = D2D1::RectF(bounds.left + 8.f, bounds.top + 25.f, bounds.right - 8.f, bounds.bottom - 7.f);

        for (size_t i = 0; i <= 4; ++i)
        {
            const float Y = Lerp(Plot.top, Plot.bottom, (float)i / 4.f);

            SetBrush(GridColor);

            renderTarget->DrawLine(D2D1::Point2F(Plot.left, Y), D2D1::Point2F(Plot.right, Y), _Brush.Get(), 1.f);
        }

        if (history.Size < 2)
            return;

        ComPtr<ID2D1PathGeometry> Geometry;
        ComPtr<ID2D1GeometrySink> Sink;

        if (FAILED(_D2DFactory->CreatePathGeometry(Geometry.GetAddressOf())) || FAILED(Geometry->Open(Sink.GetAddressOf())))
            return;

        for (size_t i = 0; i < history.Size; ++i)
        {
            const size_t Index = (history.Cursor + HistoryCount - history.Size + i) % HistoryCount;
            const double Value = std::clamp(history.Values[Index], minimum, maximum);

            const float X = Lerp(Plot.left, Plot.right, (float)i / (float)(HistoryCount - 1));
            const float Y = Lerp(Plot.bottom, Plot.top, (float)((Value - minimum) / (maximum - minimum)));

            const D2D1_POINT_2F Point = D2D1::Point2F(X, Y);

            if (i == 0)
                Sink->BeginFigure(Point, D2D1_FIGURE_BEGIN_HOLLOW);
            else
                Sink->AddLine(Point);
        }

        Sink->EndFigure(D2D1_FIGURE_END_OPEN);

        if (FAILED(Sink->Close()))
            return;

        SetBrush(SetAlpha(color, 0.18f));

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), 5.f, _StrokeStyle.Get());

        SetBrush(color);

        renderTarget->DrawGeometry(Geometry.Get(), _Brush.Get(), 1.6f, _StrokeStyle.Get());

        if (hitType == hit_type_t::None)
            return;

        const D2D1_COLOR_F LowColor    = hitType == hit_type_t::Bass ? D2D1::ColorF(0.96f, 0.78f, 0.18f) : D2D1::ColorF(0.22f, 0.76f, 1.00f);
        const D2D1_COLOR_F MediumColor = hitType == hit_type_t::Bass ? D2D1::ColorF(1.00f, 0.43f, 0.08f) : D2D1::ColorF(0.72f, 0.34f, 1.00f);
        const D2D1_COLOR_F HighColor   = hitType == hit_type_t::Bass ? D2D1::ColorF(1.00f, 0.12f, 0.06f) : D2D1::ColorF(1.00f, 0.18f, 0.48f);

        auto GetIntensityColor = [&](double intensity) noexcept
        {
            const float Amount = (float) std::clamp(intensity, 0., 1.);

            if (Amount < 0.5f)
                return Lerp(LowColor, MediumColor, Amount * 2.f);

            return Lerp(MediumColor, HighColor, (Amount - 0.5f) * 2.f);
        };

        // Draw detector events as individually color-coded spikes and label local maxima.
        for (size_t i = 0; i < history.Size; ++i)
        {
            const size_t Index = (history.Cursor + HistoryCount - history.Size + i) % HistoryCount;
            const double Value = std::clamp(history.Values[Index], 0., 1.);

            if (Value <= 0.)
                continue;

            const double Previous = i > 0 ? history.Values[(history.Cursor + HistoryCount - history.Size + i - 1) % HistoryCount] : 0.;
            const double Next = (i + 1) < history.Size ? history.Values[(history.Cursor + HistoryCount - history.Size + i + 1) % HistoryCount] : 0.;

            if ((Value < Previous) || (Value < Next))
                continue;

            const float X = Lerp(Plot.left, Plot.right, (float)i / (float)(HistoryCount - 1));
            const float Y = Lerp(Plot.bottom, Plot.top, (float)Value);

            const D2D1_COLOR_F SpikeColor = GetIntensityColor(Value);

            SetBrush(SetAlpha(SpikeColor, 0.28f));

            renderTarget->DrawLine(D2D1::Point2F(X, Plot.bottom), D2D1::Point2F(X, Y), _Brush.Get(), 5.f);

            SetBrush(SpikeColor);

            renderTarget->DrawLine(D2D1::Point2F(X, Plot.bottom), D2D1::Point2F(X, Y), _Brush.Get(), 2.f);

            const float LabelWidth = 38.f;
            const float LabelHeight = 15.f;
            const float LabelLeft = std::clamp(X - LabelWidth * 0.5f, Plot.left, std::max(Plot.left, Plot.right - LabelWidth));
            const float LabelTop = std::max(bounds.top + 22.f, Y - LabelHeight - 3.f);

            const std::wstring Intensity = std::format(L"{:.0f}%", Value * 100.);

            DrawText(renderTarget, Intensity, D2D1::RectF(LabelLeft, LabelTop, LabelLeft + LabelWidth, LabelTop + LabelHeight), _SmallFormat.Get(), SpikeColor);
        }

        // Compact intensity legend: low, medium and high confidence colors.
        const float SwatchSize   = 7.f;

        const float LegendWidth  = std::min(132.f, std::max(72.f, Plot.right - Plot.left));
        const float LegendHeight = 14.f;
        const float LegendLeft   = Plot.right - LegendWidth;
        const float LegendTop    = bounds.top + 5.f;

        const std::array<D2D1_COLOR_F, 3> LegendColors = { { LowColor, MediumColor, HighColor } };
        const std::array<const wchar_t *, 3> LegendLabels = { { L"LOW", L"MED", L"HIGH" } };

        const float ItemWidth = LegendWidth / 3.f;

        for (size_t i = 0; i < LegendColors.size(); ++i)
        {
            const float ItemLeft = LegendLeft + (float)i * ItemWidth;

            SetBrush(LegendColors[i]);

            renderTarget->FillRectangle(D2D1::RectF(ItemLeft, LegendTop + 3.f, ItemLeft + SwatchSize, LegendTop + 3.f + SwatchSize), _Brush.Get());

            DrawText(renderTarget, LegendLabels[i], D2D1::RectF(ItemLeft + SwatchSize + 2.f, LegendTop, ItemLeft + ItemWidth, LegendTop + LegendHeight), _SmallFormat.Get(), LegendColors[i]);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawPanel(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept
    {
        if ((bounds.right <= bounds.left) || (bounds.bottom <= bounds.top))
            return;

        SetBrush(PanelColor);

        renderTarget->FillRoundedRectangle(D2D1::RoundedRect(bounds, 7.f, 7.f), _Brush.Get());

        SetBrush(BorderColor);

        renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(bounds, 7.f, 7.f), _Brush.Get(), 1.f);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::CalculateBandEdges(double minFrequency, double maxFrequency, double sampleRate) noexcept
    {
        const double NyquistFrequency = sampleRate * 0.5;

        minFrequency = std::clamp(minFrequency,           1., NyquistFrequency);
        maxFrequency = std::clamp(maxFrequency, minFrequency, NyquistFrequency);

        if (maxFrequency <= minFrequency)
        {
            _BandEdges.fill(minFrequency);

            return;
        }

        const auto n = _BandEdges.size();

        const double BandRatio = std::pow(maxFrequency / minFrequency, 1. / (double) (n - 1));

        _BandEdges[0] = minFrequency;

        for (size_t i = 1; i < (n - 1); ++i)
            _BandEdges[i] = minFrequency * std::pow(BandRatio, (double) i);

        _BandEdges[n - 1] = maxFrequency;
    }

    /// <summary>
    /// 
    /// </summary>
    static double RoundToSignificantDigits(double value, int digitCount) noexcept
    {
        if (value <= 0.)
            return 0.;

        const double Magnitude = std::floor(std::log10(value));
        const double Scale     = std::pow(10., (double) (digitCount - 1) - Magnitude);

        return std::round(value * Scale) / Scale;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::CalculateBandLabels()
    {
        for (size_t i = 0; i < MonitorBandCount; ++i)
        {
            double f = RoundToSignificantDigits(std::sqrt(_BandEdges[i] * _BandEdges[i + 1]), 3);

            if (f >= 1'000.)
            {
                f /= 1'000.;

                _BandLabels[i] = (f < 10.) ? std::format(L"{:.2g}k", f) : std::format(L"{:.0f}k", f);
            }
            else
            {
                _BandLabels[i] = std::format(L"{:.0f}", f);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DrawText(ID2D1RenderTarget * renderTarget, const std::wstring & text, const D2D1_RECT_F & bounds, IDWriteTextFormat * format, const D2D1_COLOR_F & color) noexcept
    {
        if (!format || text.empty())
            return;

        const float Width  = bounds.right - bounds.left;
        const float Height = bounds.bottom - bounds.top;

        if ((Width <= 0.f) || (Height <= 0.f) || !_WriteFactory)
            return;

        ComPtr<IDWriteTextLayout> Layout;

        const HRESULT hr = _WriteFactory->CreateTextLayout(text.c_str(), (UINT32)text.size(), format, Width, Height, Layout.GetAddressOf());

        if (FAILED(hr))
            return;

        SetBrush(color);

        renderTarget->DrawTextLayout(D2D1::Point2F(bounds.left, bounds.top), Layout.Get(), _Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::SetBrush(const D2D1_COLOR_F & color) noexcept
    {
        _Brush->SetColor(color);
        _Brush->SetOpacity(1.f);
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceIndependentResources() noexcept
    {
        if (_WriteFactory && _LabelFormat && _ValueFormat && _SmallFormat)
            return S_OK;

        HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(_WriteFactory.ReleaseAndGetAddressOf()));

        if (FAILED(hr))
            return hr;

        hr = _WriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.f, L"en-us", _LabelFormat.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = _WriteFactory->CreateTextFormat(L"Consolas", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15.f, L"en-us", _ValueFormat.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = _WriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.f, L"en-us", _SmallFormat.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _LabelFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        _ValueFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _ValueFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        _SmallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _SmallFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    HRESULT effect_t::CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept
    {
        HRESULT hr = __super::CreateDeviceDependentResources(renderTarget);

        if (FAILED(hr))
            return hr;

        hr = CreateDeviceIndependentResources();

        if (FAILED(hr))
            return hr;

        if ((_RenderTarget == renderTarget) && _Brush && _StrokeStyle)
            return S_OK;

        hr = renderTarget->CreateSolidColorBrush(White, _Brush.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        const auto Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_SOLID, 0.f);

        hr = _D2DFactory->CreateStrokeStyle(Properties, nullptr, 0, _StrokeStyle.ReleaseAndGetAddressOf());

        if (FAILED(hr))
            return hr;

        _RenderTarget = renderTarget;

        return S_OK;
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::DeleteDeviceDependentResources() noexcept
    {
        _StrokeStyle.Reset();
        _Brush.Reset();

        __super::DeleteDeviceDependentResources();
    }
}

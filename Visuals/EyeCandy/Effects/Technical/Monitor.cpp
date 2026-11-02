
/** $VER: Monitor.cpp (2026.11.02) */

#include <pch.h>

#include "Monitor.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Monitor
{
    namespace
    {
        constexpr std::array<double, 17> BandEdges =
        { {
            31.5, 44.7, 63., 89.1, 125., 177., 250., 354., 500., 707., 1'000., 1'414., 2'000., 2'828., 4'000., 5'657., 8'000.
        } };

        constexpr std::array<const wchar_t *, 16> BandLabels =
        { {
            L"32", L"45", L"63", L"89", L"125", L"177", L"250", L"354", L"500", L"707", L"1k", L"1.4k", L"2k", L"2.8k", L"4k", L"5.7k"
        } };

        constexpr double HistoryInterval = 1. / 30.;
        constexpr double SignalThreshold = 1e-5; // Approximately -100 dBFS.

        const D2D1_COLOR_F CanvasColor     = D2D1::ColorF(0.032f, 0.036f, 0.041f, 1.f);
        const D2D1_COLOR_F PanelColor      = D2D1::ColorF(0.052f, 0.058f, 0.064f, 0.98f);
        const D2D1_COLOR_F InstrumentColor = D2D1::ColorF(0.070f, 0.077f, 0.085f, 1.f);
        const D2D1_COLOR_F TrackColor      = D2D1::ColorF(0.140f, 0.152f, 0.164f, 1.f);
        const D2D1_COLOR_F BorderColor     = D2D1::ColorF(0.260f, 0.280f, 0.300f, 0.88f);
        const D2D1_COLOR_F GridColor       = D2D1::ColorF(0.300f, 0.320f, 0.340f, 0.30f);
        const D2D1_COLOR_F LabelColor      = D2D1::ColorF(0.730f, 0.750f, 0.770f, 1.f);
        const D2D1_COLOR_F MutedTextColor  = D2D1::ColorF(0.560f, 0.580f, 0.600f, 1.f);
        const D2D1_COLOR_F DataColor       = D2D1::ColorF(0.900f, 0.915f, 0.930f, 1.f);
        const D2D1_COLOR_F CautionColor    = D2D1::ColorF(0.950f, 0.680f, 0.150f, 1.f);
        const D2D1_COLOR_F CriticalColor   = D2D1::ColorF(0.930f, 0.210f, 0.170f, 1.f);
        const D2D1_COLOR_F BassEventColor  = D2D1::ColorF(0.950f, 0.580f, 0.130f, 1.f);
        const D2D1_COLOR_F DrumEventColor  = D2D1::ColorF(0.920f, 0.250f, 0.190f, 1.f);
    }

    /// <summary>
    /// 
    /// </summary>
    std::wstring effect_t::GetStatus() noexcept
    {
        return std::format(L"Monitor, {} quality", Qualities[(size_t) _Quality]);
    }

    /// <summary>
    /// Clears temporal measurements while retaining device resources.
    /// </summary>
    void effect_t::Reset() noexcept
    {
        _MonitorBands.fill(0.);
        _SmoothedMonitorBands.fill(0.);
        _PreviousBands.fill(0.);

        _PeakHistory            = { };
        _RMSHistory             = { };
        _CorrelationHistory     = { };
        _FluxHistory            = { };
        _BassHitHistory         = { };
        _DrumHitHistory         = { };
        _BassEnergyHistory      = { };
        _MidEnergyHistory       = { };
        _TrebleEnergyHistory    = { };

        _BassDetector           = 0.;
        _DrumDetector           = 0.;

        _SpectralCentroid       = 0.;
        _SpectralFlatness       = 0.;
        _MonoCompatibility      = 1.;
        _ClippedSampleRatio     = 0.;
        _DCOffset               = 0.;
        _BassCorrelation        = 0.;
        _TransientRate          = 0.;

        _PhaseEnergy            = 0.;
        _BeatAverage            = 0.;
        _BeatEnvelope           = 0.;
        _BeatPulse              = 0.;
        _BeatCooldown           = 0.;
        _PreviousBassEnvelope   = 0.;
        _DrumCooldown           = 0.;
        _PreviousRMS            = 0.;
        _BassHitEvent           = false;
        _DrumHitEvent           = false;
        _BassHitIntensity       = 0.;
        _DrumHitIntensity       = 0.;

        _ChannelBalance         = 0.;

        _GaugePeakHold          = { { -60., -60., 0., 0., -1., 0., 0., 0., 0., -18., 31.5, 0., 0., 0., -10., -1., 0. } };
        _GaugePeakHoldElapsed.fill(0.);

        _HistoryElapsed         = 0.;

        _Peak                   = 0.;
        _RMS                    = 0.;

        _CrestFactor            = 0.;
        _StereoWidth            = 0.;
        _PhaseEnergy            = 0.;
        _SpectralFlux           = 0.;

        _BassEnergy             = 0.;
        _MidEnergy              = 0.;
        _TrebleEnergy           = 0.;

        _Correlation            = 0.;

        _SmoothedPeak           = 0.;
        _SmoothedRMS            = 0.;

        _SmoothedCrestFactor    = 0.;
        _SmoothedStereoWidth    = 0.;
        _SmoothedPhaseEnergy    = 0.;
        _SmoothedSpectralFlux   = 0.;

        _SmoothedBass           = 0.;
        _SmoothedMid            = 0.;
        _SmoothedTreble         = 0.;

        _SmoothedPhaseEnergy    = 0.;
        _SmoothedCorrelation    = 0.;

        _SmoothedBands.fill(0.);
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

        const D2D1_RECT_F History   = D2D1::RectF(Margin, Margin, Margin + HistoryWidth, Margin + ContentHeight);
        const D2D1_RECT_F Gauges    = D2D1::RectF(RightLeft, Margin, RightLeft + RightWidth, Margin + GaugeHeight);
        const D2D1_RECT_F Histogram = D2D1::RectF(RightLeft, Gauges.bottom + Gap, RightLeft + RightWidth, Gauges.bottom + Gap + HistogramHeight);

        RenderHistory(renderTarget, History);
        RenderGauges(renderTarget, Gauges);
        RenderHistogram(renderTarget, Histogram);

        RenderFrame(renderTarget, BorderColor);
    }

    /// <summary>
    /// Calculates time-domain level, stereo and crest measurements from the selected channels.
    /// </summary>
    void effect_t::AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels) noexcept
    {
        _HasAudio = false;

        _Peak               = 0.;
        _RMS                = 0.;

        _CrestFactor        = 0.;
        _StereoWidth        = 0.;
        _PhaseEnergy        = 0.;
        _SpectralFlux       = 0.;

        _SpectralCentroid   = BandEdges.front();
        _SpectralFlatness   = 0.;
        _MonoCompatibility  = 0.;
        _ClippedSampleRatio = 0.;
        _DCOffset           = 0.;
        _BassCorrelation    = 1.;
        _TransientRate      = 0.;

        _ChannelBalance     = 0.;
        _Correlation        = 0.;

        _Bands.fill(0.);
        _MonitorBands.fill(0.);

        if (chunk.is_empty())
            return;

        const size_t ChannelCount = chunk.get_channel_count();
        const size_t FrameCount   = chunk.get_sample_count();

        if ((FrameCount < 16) || (channels[0] >= ChannelCount) || (channels[1] >= ChannelCount))
            return;

        _SampleRate = std::max(1., (double) chunk.get_sample_rate());

        double Peak = 0.;

        double SumLeftSquares  = 0.;
        double SumRightSquares = 0.;
        double SumCross        = 0.;

        double SumSquares = 0.;

        double SumMidSquares  = 0.;
        double SumSideSquares = 0.;

        double SumLeft  = 0.;
        double SumRight = 0.;

        double BassLeft  = 0.;
        double BassRight = 0.;

        double SumBassLeftSquares  = 0.;
        double SumBassRightSquares = 0.;
        double SumBassCross        = 0.;

        size_t ClippedSampleCount  = 0;

        const size_t MaxFrames = std::min<size_t>(FrameCount, (_Quality == Quality::High) ? FrameCount : (_Quality == Quality::Low) ? FrameCount / 4 : FrameCount / 2);
        const size_t FrameStep = std::max<size_t>(1, FrameCount / MaxFrames);

        const double BassCoefficient  = 1. - std::exp(-TwoPi * BassCutoff / std::max(1., _SampleRate / (double) FrameStep));

        {
            const audio_sample * Frames = chunk.get_data();

            if (_Samples.size() < MaxFrames)
                _Samples.resize(MaxFrames);

            auto Samples = std::span<double>(_Samples.data(), MaxFrames);

            for (size_t i = 0; i < MaxFrames; ++i)
            {
                const size_t FrameIndex = std::min(i * FrameStep, FrameCount - 1);

                const audio_sample * Frame = &Frames[FrameIndex * ChannelCount];

                const double Left  = ClampAudio(Frame[channels[0]]);
                const double Right = ClampAudio(Frame[channels[1]]);

                const double Mid   = 0.5 * (Left + Right);
                const double Side  = 0.5 * (Left - Right);

                Peak = std::max(Peak, std::max(std::abs(Left), std::abs(Right)));

                SumLeftSquares      += Left  * Left;
                SumRightSquares     += Right * Right;
                SumCross            += Left  * Right;

                SumSquares          += 0.5 * ((Left  * Left) + (Right * Right));

                SumMidSquares       += Mid  * Mid;
                SumSideSquares      += Side * Side;

                SumLeft             += Left;
                SumRight            += Right;

                BassLeft            += (Left  - BassLeft)  * BassCoefficient;
                BassRight           += (Right - BassRight) * BassCoefficient;

                SumBassLeftSquares  += BassLeft  * BassLeft;
                SumBassRightSquares += BassRight * BassRight;
                SumBassCross        += BassLeft  * BassRight;

                if (std::abs(Left) >= 0.999 || std::abs(Right) >= 0.999)
                    ++ClippedSampleCount;

                Samples[i] = Mid;
            }

            AnalyzeSpectrum(Samples, _SampleRate / (double) FrameStep);
        }

        {
            _Peak = Clamp(Peak);
            _RMS  = Clamp(std::sqrt(SumSquares / (double) MaxFrames));
        }
        {
            const double CorrelationDenominator = std::sqrt(SumLeftSquares * SumRightSquares);

            _Correlation = (CorrelationDenominator > Epsilon) ? std::clamp(SumCross / CorrelationDenominator, -1., 1.) : 0.;
        }

        const double MidRMS  = std::sqrt(SumMidSquares  / (double) MaxFrames);
        const double SideRMS = std::sqrt(SumSideSquares / (double) MaxFrames);

        _StereoWidth = (MidRMS + SideRMS) > Epsilon ? Clamp(SideRMS / (MidRMS + SideRMS)) : 0.;

        {
            const double TotalPhaseEnergy = SumMidSquares + SumSideSquares;

            _PhaseEnergy = (TotalPhaseEnergy) > Epsilon ? Clamp(SumSideSquares / TotalPhaseEnergy) : 0.;
        }

        {
            const double LeftRMS  = std::sqrt(SumLeftSquares  / (double) MaxFrames);
            const double RightRMS = std::sqrt(SumRightSquares / (double) MaxFrames);

            // Negative values indicate left dominance; positive values indicate right dominance.
            _ChannelBalance = std::clamp(20. * std::log10(std::max(RightRMS, 0.000001) / std::max(LeftRMS, 0.000001)), -18., 18.);
        }

        {
            _MonoCompatibility  = (_RMS > Epsilon) ? Clamp(MidRMS / _RMS) : 0.;
            _ClippedSampleRatio =      Clamp((double) ClippedSampleCount  / (double) MaxFrames);
            _DCOffset           = std::clamp(500. * ((SumLeft + SumRight) / (double) MaxFrames), -10., 10.);
        }

        {
            const double BassCorrelationDenominator = std::sqrt(SumBassLeftSquares * SumBassRightSquares);

            _BassCorrelation = (BassCorrelationDenominator > Epsilon) ? std::clamp(SumBassCross / BassCorrelationDenominator, -1., 1.) : 0.;
        }

        {
            _CrestFactor = (_RMS > Epsilon) ? std::clamp(20. * std::log10(std::max(_Peak / _RMS, 1.)), 0., 30.) : 0.;
        }

        _HasAudio = (_Peak > SignalThreshold) || (_RMS > SignalThreshold);
    }

    /// <summary>
    /// Integrates a Hann-windowed DFT over octave-like bands and computes positive spectral flux.
    /// </summary>
    void effect_t::AnalyzeSpectrum(const std::span<double> & samples, double sampleRate) noexcept
    {
        if (samples.size() < 16)
            return;

        const size_t SampleCount         = samples.size();
        const double FrequencyResolution = sampleRate / (double) SampleCount;
        const size_t MaximumBin          = std::min<size_t>(SampleCount / 2, (size_t) std::floor(BandEdges.back() / FrequencyResolution));

        if (_AnalysisWindow.size() != SampleCount)
        {
            _AnalysisWindow.resize(SampleCount);

            double WindowSum = 0.;

            for (size_t i = 0; i < SampleCount; ++i)
            {
                _AnalysisWindow[i] = 0.5 - (0.5 * std::cos(TwoPi * (double) i / (double) (SampleCount - 1)));

                WindowSum += _AnalysisWindow[i];
            }

            _AnalysisWindowGain = WindowSum / (double)SampleCount;
        }

        const double CoherentGain = _AnalysisWindowGain;

        std::array<double, MonitorBandCount> SumSquares{ };
        std::array<size_t, MonitorBandCount> BinCounts{ };

        for (size_t bin = 1; bin <= MaximumBin; ++bin)
        {
            const double Frequency = (double)bin * FrequencyResolution;
            const auto Upper = std::upper_bound(BandEdges.begin(), BandEdges.end(), Frequency);

            if ((Upper == BandEdges.begin()) || (Upper == BandEdges.end()))
                continue;

            const size_t Band = (size_t)std::distance(BandEdges.begin(), Upper) - 1;

            double Real = 0.;
            double Imaginary = 0.;

            double OscillatorReal = 1.;
            double OscillatorImaginary = 0.;

            const double StepAngle     = -TwoPi * (double)bin / (double)SampleCount;
            const double StepReal      = std::cos(StepAngle);
            const double StepImaginary = std::sin(StepAngle);

            for (size_t i = 0; i < SampleCount; ++i)
            {
                const double Value = samples[i] * _AnalysisWindow[i];

                Real      += Value * OscillatorReal;
                Imaginary += Value * OscillatorImaginary;

                const double NextReal = OscillatorReal * StepReal - OscillatorImaginary * StepImaginary;

                OscillatorImaginary = OscillatorReal * StepImaginary + OscillatorImaginary * StepReal;
                OscillatorReal      = NextReal;
            }

            const double Amplitude = 2. * std::sqrt((Real * Real) + (Imaginary * Imaginary)) / ((double)SampleCount * CoherentGain);

            SumSquares[Band] += Amplitude * Amplitude;

            ++BinCounts[Band];
        }

        double PositiveDifference = 0.;
        double ReferenceEnergy = Epsilon;

        for (size_t i = 0; i < MonitorBandCount; ++i)
        {
            _MonitorBands[i] = BinCounts[i] > 0 ? Clamp(std::sqrt(SumSquares[i] / (double)BinCounts[i])) : 0.;

            PositiveDifference += std::max(0., _MonitorBands[i] - _PreviousBands[i]);
            ReferenceEnergy    += _PreviousBands[i];

            _PreviousBands[i] = _MonitorBands[i];
        }

        constexpr double PowerFloor = 1e-12;

        double TotalPower = 0.;
        double LogPowerSum = 0.;

        {
            double WeightedFrequency = 0.;

            for (size_t i = 0; i < MonitorBandCount; ++i)
            {
                const double CenterFrequency = std::sqrt(BandEdges[i] * BandEdges[i + 1]);
                const double Power = std::max(_MonitorBands[i] * _MonitorBands[i], PowerFloor);

                TotalPower  += Power;
                LogPowerSum += std::log(Power);

                WeightedFrequency += CenterFrequency * Power;
            }

            _SpectralCentroid = (TotalPower > PowerFloor) ? std::clamp(WeightedFrequency / TotalPower, BandEdges.front(), BandEdges.back()) : BandEdges.front();
        }

        {
            const double ArithmeticMean = TotalPower / (double) MonitorBandCount;
            const double GeometricMean  = std::exp(LogPowerSum / (double) MonitorBandCount);

            _SpectralFlatness = ArithmeticMean > PowerFloor ? Clamp(GeometricMean / ArithmeticMean) : 0.;
        }

        _SpectralFlux = Clamp(PositiveDifference / ReferenceEnergy);

        // Preserve the base class' eight-band controls by energy-combining adjacent monitor bands.
        {
            for (size_t i = 0; i < FrequencyBandCount; ++i)
                _Bands[i] = std::sqrt(0.5 * ((_MonitorBands[i * 2] * _MonitorBands[i * 2]) + (_MonitorBands[i * 2 + 1] * _MonitorBands[i * 2 + 1])));
        }

        // Combine band amplitudes as RMS energy. Arithmetic averaging suppresses narrow-band content and makes the regional histories appear incorrectly flat.
        {
            _BassEnergy   = std::sqrt(((_Bands[0] * _Bands[0]) + (_Bands[1] * _Bands[1]) + (_Bands[2] * _Bands[2])) / 3.);
            _MidEnergy    = std::sqrt(((_Bands[3] * _Bands[3]) + (_Bands[4] * _Bands[4]) + (_Bands[5] * _Bands[5])) / 3.);
            _TrebleEnergy = std::sqrt(((_Bands[6] * _Bands[6]) + (_Bands[7] * _Bands[7]))                           / 2.);
        }
    }

    /// <summary>
    /// Applies asymmetric ballistics: fast attack and slower release for readable instrumentation.
    /// </summary>
    void effect_t::Update(double deltaTime) noexcept
    {
        _AnimationTime += deltaTime;

        if (!_HasAudio)
        {
            _Peak = 0.;
            _RMS = 0.;
            _StereoWidth = 0.;
            _Correlation = 0.;
            _BassEnergy = 0.;
            _MidEnergy = 0.;
            _TrebleEnergy = 0.;
            _PhaseEnergy = 0.;
            _SpectralFlux = 0.;
            _CrestFactor = 0.;

            _SmoothedPeak = 0.;
            _SmoothedRMS = 0.;

            _SmoothedCrestFactor = 0.;
            _SmoothedStereoWidth = 0.;

            _SmoothedBass = 0.;
            _SmoothedMid = 0.;
            _SmoothedTreble = 0.;

            _SmoothedCorrelation = 0.;
            _SmoothedSpectralFlux = 0.;
            _SmoothedPhaseEnergy = 0.;

            _Bands.fill(0.);
            _SmoothedBands.fill(0.);
            _MonitorBands.fill(0.);
            _SmoothedMonitorBands.fill(0.);
            _PreviousBands.fill(0.);

            _BeatAverage = 0.;
            _BeatEnvelope = 0.;
            _BeatPulse = 0.;
            _BeatCooldown = 0.;
            _PreviousBassEnvelope = 0.;
            _BassDetector = 0.;
            _DrumDetector = 0.;
            _DrumCooldown = 0.;
            _PreviousRMS = 0.;
            _BassHitEvent = false;
            _DrumHitEvent = false;
            _BassHitIntensity = 0.;
            _DrumHitIntensity = 0.;

            _ChannelBalance = 0.;
            _SpectralCentroid = BandEdges.front();
            _SpectralFlatness = 0.;
            _MonoCompatibility = 0.;
            _ClippedSampleRatio = 0.;
            _DCOffset = 0.;
            _BassCorrelation = 0.;
            _TransientRate = 0.;

            _GaugePeakHold = { { -60., -60., 0., 0., 0., 0., 0., 0., 0., 0., BandEdges.front(), 0., 0., 0., 0., 0., 0. } };
            _GaugePeakHoldElapsed.fill(0.);

            RecordHistory(deltaTime);

            return;
        }

        _SmoothedPeak         = ExponentialSmoothing(_SmoothedPeak,         _Peak,         deltaTime, (_Peak > _SmoothedPeak) ? 24. : 2.4);
        _SmoothedRMS          = ExponentialSmoothing(_SmoothedRMS,          _RMS,          deltaTime, (_RMS  > _SmoothedRMS)  ? 14. : 2.2);

        _SmoothedCrestFactor  = ExponentialSmoothing(_SmoothedCrestFactor,  _CrestFactor,  deltaTime, 4.0);
        _SmoothedStereoWidth  = ExponentialSmoothing(_SmoothedStereoWidth,  _StereoWidth,  deltaTime, 5.0);

        _SmoothedCorrelation  = ExponentialSmoothing(_SmoothedCorrelation,  _Correlation,  deltaTime, 5.0);
        _SmoothedSpectralFlux = ExponentialSmoothing(_SmoothedSpectralFlux, _SpectralFlux, deltaTime, (_SpectralFlux > _SmoothedSpectralFlux) ? 12. : 3.0);
        _SmoothedPhaseEnergy  = ExponentialSmoothing(_SmoothedPhaseEnergy,  _PhaseEnergy,  deltaTime, (_PhaseEnergy  > _SmoothedPhaseEnergy)  ? 12. : 3.0);

        for (size_t i = 0; i < FrequencyBandCount; ++i)
            _SmoothedBands[i] = ExponentialSmoothing(_SmoothedBands[i], _Bands[i], deltaTime, _Bands[i] > _SmoothedBands[i] ? 15. : 3.0);

        for (size_t i = 0; i < MonitorBandCount; ++i)
            _SmoothedMonitorBands[i] = ExponentialSmoothing(_SmoothedMonitorBands[i], _MonitorBands[i], deltaTime, _MonitorBands[i] > _SmoothedMonitorBands[i] ? 15. : 3.0);

        _SmoothedBass   = ExponentialSmoothing(_SmoothedBass,   _BassEnergy,   deltaTime, 5.0);
        _SmoothedMid    = ExponentialSmoothing(_SmoothedMid,    _MidEnergy,    deltaTime, 5.0);
        _SmoothedTreble = ExponentialSmoothing(_SmoothedTreble, _TrebleEnergy, deltaTime, 5.0);

        // Bass events use a slow adaptive floor and the positive slope of the low-frequency envelope.
        _BeatAverage  = ExponentialSmoothing(_BeatAverage,  _BassEnergy, deltaTime, 1.25);
        _BeatEnvelope = ExponentialSmoothing(_BeatEnvelope, _BassEnergy, deltaTime, _BassEnergy > _BeatEnvelope ? 24. : 5.0);

        _BeatCooldown = std::max(0., _BeatCooldown - deltaTime);

        const double BassRise       = std::max(0., _BeatEnvelope - _PreviousBassEnvelope);
        const double BassThreshold  = 0.006 + (_BeatAverage * 0.10);
        const double BassConfidence = Clamp(((BassRise - BassThreshold) / std::max(0.01, BassThreshold * 2.5)) * _BassDetectorSensitivity);

        _PreviousBassEnvelope = _BeatEnvelope;
        _BassDetector         = std::max(0., _BassDetector - deltaTime * 4.0);

        if ((_BeatCooldown <= 0.) && (BassConfidence >= 0.35) && (_BassEnergy >= _BeatAverage * 1.05))
        {
            _BassDetector     = 1.;
            _BassHitEvent     = true;
            _BassHitIntensity = std::max(_BassHitIntensity, BassConfidence);

            _BeatPulse    = 1.;
            _BeatCooldown = 0.12;
        }

        _BeatPulse    = std::max(0., _BeatPulse - deltaTime * 5.0);
        _BassDetector = std::max(_BassDetector, BassConfidence);

        // Drum events combine broadband spectral change, level onset and bass evidence.
        _DrumCooldown = std::max(0., _DrumCooldown - deltaTime);

        const double RMSRise        = std::max(0., _RMS - _PreviousRMS);
        const double DrumConfidence = Clamp(((_SpectralFlux * 0.62) + (RMSRise * 8.0) + (BassConfidence * 0.28)) * _DrumDetectorSensitivity);

        _PreviousRMS  = _RMS;
        _DrumDetector = std::max(0., _DrumDetector - deltaTime * 5.5);

        if ((_DrumCooldown <= 0.) && (DrumConfidence >= 0.42) && (_Peak >= 0.02))
        {
            _DrumDetector     = 1.;
            _DrumHitEvent     = true;
            _DrumHitIntensity = std::max(_DrumHitIntensity, DrumConfidence);
            _DrumCooldown     = 0.085;
        }

        _DrumDetector = std::max(_DrumDetector, DrumConfidence);

        const double TransientDecay = std::exp(-deltaTime / 3.0);

        _TransientRate *= TransientDecay;

        if (_DrumHitEvent)
            _TransientRate += (1. - TransientDecay) / std::max(deltaTime, 1. / 240.);

        _TransientRate = std::clamp(_TransientRate, 0., 12.);

        const std::array<double, 17> GaugeValues =
        { {
            AmplitudeToDecibels(_SmoothedPeak),
            AmplitudeToDecibels(_SmoothedRMS),
            _SmoothedCrestFactor,
            _SmoothedStereoWidth * 100.,
            _SmoothedCorrelation,
            _SmoothedPhaseEnergy,
            _SmoothedSpectralFlux,
            _BassDetector,
            _DrumDetector,
            _ChannelBalance,
            _SpectralCentroid,
            _SpectralFlatness,
            _MonoCompatibility  * 100.,
            _ClippedSampleRatio * 100.,
            _DCOffset,
            _BassCorrelation,
            _TransientRate
        } };

        constexpr std::array<double, 17> GaugeMinimums = { { -60., -60., 0., 0., -1., 0., 0., 0., 0., -18., 31.5, 0., 0., 0., -10., -1., 0. } };

        constexpr double PeakHoldDuration = 1.5;
        constexpr double PeakHoldRelease = 0.18;

        for (size_t i = 0; i < _GaugePeakHold.size(); ++i)
        {
            if (GaugeValues[i] >= _GaugePeakHold[i])
            {
                _GaugePeakHold[i] = GaugeValues[i];
                _GaugePeakHoldElapsed[i] = 0.;
            }
            else
            {
                _GaugePeakHoldElapsed[i] += deltaTime;

                if (_GaugePeakHoldElapsed[i] >= PeakHoldDuration)
                    _GaugePeakHold[i] = std::max(GaugeValues[i], _GaugePeakHold[i] - PeakHoldRelease * deltaTime * std::max(1., std::abs(_GaugePeakHold[i] - GaugeMinimums[i])));
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
            Push(_BassEnergyHistory,   AmplitudeToDecibels(_SmoothedBass));
            Push(_MidEnergyHistory,    AmplitudeToDecibels(_SmoothedMid));
            Push(_TrebleEnergyHistory, AmplitudeToDecibels(_SmoothedTreble));

            _BassHitEvent = false;
            _DrumHitEvent = false;
            _BassHitIntensity = 0.;
            _DrumHitIntensity = 0.;
        }
    }

    void effect_t::RenderGauges(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept
    {
        const float Width  = bounds.right  - bounds.left;
        const float Height = bounds.bottom - bounds.top;

        if ((Width < 6.f) || (Height < 20.f))
            return;

        DrawPanel(renderTarget, bounds);

        const std::array<gauge_t, 15> Gauges =
        { {
            { L"Peak",              L"dBFS",        AmplitudeToDecibels(_SmoothedPeak), -60.,   0., 1, _GaugePeakHold[ 0], DataColor },
            { L"RMS",               L"dBFS",        AmplitudeToDecibels(_SmoothedRMS),  -60.,   0., 1, _GaugePeakHold[ 1], DataColor },

            { L"Crest Factor",      L"dB",          _SmoothedCrestFactor,                 0.,  30., 1, _GaugePeakHold[ 2], DataColor },
            { L"Stereo Width",      L"%",           _SmoothedStereoWidth * 100.,          0., 100., 0, _GaugePeakHold[ 3], DataColor },
            { L"Phase Energy",      L"norm",        _SmoothedPhaseEnergy,                 0.,   1., 2, _GaugePeakHold[ 5], DataColor },
            { L"Spectral Flux",     L"norm",        _SmoothedSpectralFlux,                0.,   1., 2, _GaugePeakHold[ 6], DataColor },

            { L"Bass Hit",          L"%",           _BassDetector * 100.,                 0.,   1., 0, _GaugePeakHold[ 7], _BassDetector >= 0.35 ? BassEventColor : DataColor },
            { L"Drum Hit",          L"%",           _DrumDetector * 100.,                 0.,   1., 0, _GaugePeakHold[ 8], _DrumDetector >= 0.42 ? DrumEventColor : DataColor },

            { L"Spectral Centroid", L"Hz",          _SpectralCentroid,                31.5, 8'000., 0, _GaugePeakHold[10], DataColor },
            { L"Spectral Flatness", L"norm",        _SpectralFlatness,                    0.,   1., 2, _GaugePeakHold[11], DataColor },
            { L"Mono Compat.",      L"%",           _MonoCompatibility * 100.,            0., 100., 1, _GaugePeakHold[12], DataColor },
            { L"Clipped Samples",   L"%",           _ClippedSampleRatio * 100.,           0., 100., 2, _GaugePeakHold[13], _ClippedSampleRatio > 0. ? CriticalColor : DataColor },
            { L"DC Offset",         L"mFS",         _DCOffset,                          -10.,  10., 2, _GaugePeakHold[14], std::abs(_DCOffset) > 1. ? CautionColor : DataColor },
            { L"Bass Correlation",  L"r",           _BassCorrelation,                    -1.,   1., 2, _GaugePeakHold[15], _BassCorrelation < 0. ? CriticalColor : DataColor },
            { L"Transient Rate",    L"hits/s",      _TransientRate,                       0.,  12., 1, _GaugePeakHold[16], DataColor }
        } };

        const std::array<gauge_t, 2> HorizontalGauges =
        { {
            { L"L/R Balance",       L"dB",          _ChannelBalance,                    -18.,  18., 1, _GaugePeakHold[9], DataColor },
            { L"Correlation",       L"r",           _SmoothedCorrelation,                -1.,   1., 2, _GaugePeakHold[4], _SmoothedCorrelation < 0. ? CriticalColor : DataColor }
        } };

        const float Padding = std::min(12.f, std::max(2.f, Width * 0.012f));
        const float Gap     = std::min(8.f, std::max(1.f, Width * 0.006f));

        const float HorizontalAreaHeight = std::max(42.f, Height * 0.22f);
        const float VerticalAreaHeight   = std::max(1.f, Height - Padding * 2.f - HorizontalAreaHeight - Gap);

        constexpr size_t ColumnCount = 8;
        constexpr size_t RowCount = 2;

        const float AvailableWidth  = std::max(1.f, Width - Padding * 2.f - Gap * (float)(ColumnCount - 1));
        const float AvailableHeight = std::max(1.f, VerticalAreaHeight - Gap * (float)(RowCount - 1));

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

        const float HorizontalTop         = bounds.bottom - Padding - HorizontalAreaHeight;
        const float HorizontalGaugeHeight = std::max(1.f, (HorizontalAreaHeight - Gap) * 0.5f);

        for (size_t i = 0; i < HorizontalGauges.size(); ++i)
        {
            const float Top = HorizontalTop + (float)i * (HorizontalGaugeHeight + Gap);

            RenderHorizontalGauge(renderTarget, D2D1::RectF(bounds.left + Padding, Top, bounds.right - Padding, Top + HorizontalGaugeHeight), HorizontalGauges[i]);
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
            const float Amount     = (float)i / 4.f;
            const float Y          = Lerp(Track.bottom, Track.top, Amount);
            const float TickLength = (i == 0 || i == 4) ? 8.f : 5.f;

            SetBrush(MutedTextColor);

            renderTarget->DrawLine(D2D1::Point2F(Track.left - TickLength, Y), D2D1::Point2F(Track.left - 2.f, Y), _Brush.Get(), 1.f);
            renderTarget->DrawLine(D2D1::Point2F(Track.right + 2.f, Y), D2D1::Point2F(Track.right + TickLength, Y), _Brush.Get(), 1.f);
        }

        const std::wstring Value = gauge.Decimals == 2 ? std::format(L"{:.2f} {}", gauge.Value, gauge.Unit) : std::format(L"{:.1f} {}", gauge.Value, gauge.Unit);

        DrawText(renderTarget, Value, D2D1::RectF(bounds.left + 3.f, bounds.bottom - ValueHeight, bounds.right - 3.f, bounds.bottom - 2.f), _ValueFormat.Get(), White);
    }

    /// <summary>
    /// 
    /// </summary>
    void effect_t::RenderHorizontalGauge(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const gauge_t & gauge) noexcept
    {
        const float Width = bounds.right - bounds.left;
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

        const double Normalized     = std::clamp((gauge.Value - gauge.Minimum) / (gauge.Maximum - gauge.Minimum), 0., 1.);
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
            const float X = Lerp(Track.left, Track.right, (float)i / 4.f);
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
        const float BarSlot    = (Right - Left) / (float)MonitorBandCount;
        const float BarWidth   = BarSlot * 0.72f;

        for (int db = -60; db <= 0; db += 12)
        {
            const float Amount = (float)((db - MinDecibels) / (MaxDecibels - MinDecibels));
            const float Y = Bottom - Amount * PlotHeight;

            SetBrush(GridColor);

            renderTarget->DrawLine(D2D1::Point2F(Left, Y), D2D1::Point2F(Right, Y), _Brush.Get(), 1.f);

            DrawText(renderTarget, std::format(L"{}", db), D2D1::RectF(bounds.left + 4.f, Y - 8.f, Left - 4.f, Y + 9.f), _SmallFormat.Get(), MutedTextColor);
        }

        for (size_t i = 0; i < MonitorBandCount; ++i)
        {
            const double Decibels = AmplitudeToDecibels(_SmoothedMonitorBands[i]);
            const float Amount = (float)std::clamp((Decibels - MinDecibels) / (MaxDecibels - MinDecibels), 0., 1.);
            const float X = Left + (float)i * BarSlot + (BarSlot - BarWidth) * 0.5f;

            const D2D1_RECT_F Bar = D2D1::RectF(X, Bottom - Amount * PlotHeight, X + BarWidth, Bottom);

            const D2D1_COLOR_F BarColor = Decibels >= -1. ? CriticalColor : Decibels >= -6. ? CautionColor : DataColor;

            SetBrush(BarColor);

            renderTarget->FillRectangle(Bar, _Brush.Get());

            DrawText(renderTarget, BandLabels[i], D2D1::RectF(X - BarSlot * 0.15f, Bottom + 3.f, X + BarWidth + BarSlot * 0.15f, bounds.bottom - 3.f), _SmallFormat.Get(), LabelColor);
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

                case 5: RenderHistoryGraph(renderTarget, Graph, L"Bass Energy (dBFS)",      _BassEnergyHistory, -60., 0., DataColor); break;
                case 6: RenderHistoryGraph(renderTarget, Graph, L"Mid Energy (dBFS)",       _MidEnergyHistory, -60., 0., DataColor); break;
                case 7: RenderHistoryGraph(renderTarget, Graph, L"Treble Energy (dBFS)",    _TrebleEnergyHistory, -60., 0., DataColor); break;
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
            const float Amount = (float)std::clamp(intensity, 0., 1.);

            if (Amount < 0.5f)
                return LerpColor(LowColor, MediumColor, Amount * 2.f);

            return LerpColor(MediumColor, HighColor, (Amount - 0.5f) * 2.f);
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
        if (_WriteFactory && _TitleFormat && _LabelFormat && _ValueFormat && _SmallFormat)
            return S_OK;

        HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(_WriteFactory.ReleaseAndGetAddressOf()));

        if (FAILED(hr))
            return hr;

        hr = _WriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 23.f, L"en-us", _TitleFormat.ReleaseAndGetAddressOf());

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

        _TitleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
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

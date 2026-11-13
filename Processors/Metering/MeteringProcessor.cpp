
/** $VER: MeteringProcessor.cpp (2026.11.05) P. Stuer - Implements an audio processor for the metering visualization. **/

#include <pch.h>

#include "MeteringProcessor.h"
#include "Support.h"

namespace Metering
{
    /// <summary>
    /// 
    /// </summary>
    void processor_t::Configure(const graph_options_t * options) noexcept
    {
        __super::Configure(options);
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::Reset() noexcept
    {
        __super::Reset();

        // Peak/RMS Meter
        {
            ResetMeasurements(ResetOptions::Value);
            ResetRMSDependentValues();

            InitializeMeasurements((uint32_t) Channels::ConfigStereo);
        }

        // Balance/Correlation Meter
        {
            _Balance = 0.5;
            _Phase   = 0.5;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::Process(const audio_chunk & chunk) noexcept
    {
        if (chunk.is_empty())
            return;

        const uint32_t AvailableChannels = chunk.get_channel_config();                          // Mask containing the channels in the audio chunk.
        const uint32_t SelectedChannels  = AvailableChannels & _Options->_ActiveChannelMask;    // Mask containing the channels selected by the user for visualization.

        if (SelectedChannels == 0)
            return; // None of the selected channels are present in this chunk.

        InitializeMeasurements(SelectedChannels);

        constexpr size_t MaximumChannelCount = 32;
        constexpr size_t BalanceChannelCount = 2;

        // Per-channel mapping structure.
        struct channel_t
        {
            uint32_t SampleOffset {};   // Position of the channel within an audio frame
            size_t MeasurementIndex {}; // Corresponding position in _PeakMeasurements.
        };

        std::array<channel_t, MaximumChannelCount> MeterChannels {};  // Mappings for channels used by the meter.

        size_t MeterChannelCount = 0;

        std::array<uint32_t, BalanceChannelCount> BalanceOffsets {};        // Store the offsets of the samples used by the balance pair.

        size_t BalanceOffsetCount = 0;

        // Build the channel-to-sample-offset mappings once per chunk.
        {
            const uint32_t ActiveChannelMask = _Options->_ActiveChannelMask;
            const size_t ChannelPairIndex = (size_t) _Options->_ChannelPair;

            const uint32_t PairedChannelMask = ChannelPairs[ChannelPairIndex];

            uint32_t ChannelMask  = AvailableChannels;
            uint32_t ChannelBit   = 1;
            uint32_t SampleOffset = 0;

            size_t MeasurementIndex = 0;

            while ((ChannelMask != 0) && (ChannelBit != 0))
            {
                if ((ChannelMask & ChannelBit) != 0)
                {
                    const bool IsActive = (ActiveChannelMask & ChannelBit) != 0;

                    if (IsActive && (MeasurementIndex < _Measurements.size()))
                    {
                        MeterChannels[MeterChannelCount++] =
                        {
                            SampleOffset,
                            MeasurementIndex
                        };

                        if (((PairedChannelMask & ChannelBit) != 0) && (BalanceOffsetCount < BalanceOffsets.size()))
                            BalanceOffsets[BalanceOffsetCount++] = SampleOffset;

                        ++MeasurementIndex;
                    }

                    ++SampleOffset;
                    ChannelMask &= ~ChannelBit;
                }

                ChannelBit <<= 1;
            }
        }

        const audio_sample * const Frames = chunk.get_data();
        const size_t FrameCount           = chunk.get_sample_count();
        const uint32_t ChannelCount       = chunk.get_channel_count();

        const audio_sample * Frame = Frames;

        for (size_t FrameIndex = 0; FrameIndex < FrameCount; FrameIndex += (size_t) ChannelCount, Frame += ChannelCount)
        {
            for (size_t ChannelIndex = 0; ChannelIndex < MeterChannelCount; ++ChannelIndex)
            {
                const channel_t & Channel = MeterChannels[ChannelIndex];

                const double Sample    = (double) Frame[Channel.SampleOffset];
                const double Magnitude = std::abs(Sample);

                auto & Measurement = _Measurements[Channel.MeasurementIndex];

                Measurement.Peak      = std::max(Measurement.Peak, Magnitude);
                Measurement.RMSTotal += Sample * Sample;
            }

            // Reset the balance samples for every frame.
            std::array<double, BalanceChannelCount> BalanceSamples {};

            for (size_t ChannelIndex = 0; ChannelIndex < BalanceOffsetCount; ++ChannelIndex)
                BalanceSamples[ChannelIndex] = (double) Frame[BalanceOffsets[ChannelIndex]];

            const double Left  = BalanceSamples[0];
            const double Right = BalanceSamples[1];

            _LeftSumSquared  += Left  * Left;
            _RightSumSquared += Right * Right;

            const double Mid  = (Left + Right) * 0.5;
            const double Side = (Left - Right) * 0.5;

            _MidSumSquared  += Mid  * Mid;
            _SideSumSquared += Side * Side;
        }

        _RMSFrameCount  += FrameCount;
        _RMSTimeElapsed += chunk.get_duration();

        // Normalize and smooth the peak values. https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
        for (auto & Measurement : _Measurements)
        {
            Measurement.Peak           = ToDecibel(Measurement.Peak);
            Measurement.NormalizedPeak = SmoothValue(NormalizeValue(Measurement.Peak), Measurement.NormalizedPeak);
        }

        // Keep accumulating until the configured RMS window has elapsed.
        if (_RMSTimeElapsed < _State._RMSWindow)
            return;

        const double RMSFrameCount = (double) _RMSFrameCount;

        // Normalize and smooth the RMS values. https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
        for (auto & m : _Measurements)
        {
            const double MeanSquare = m.RMSTotal / RMSFrameCount;

            m.RMS           = ToDecibel(std::sqrt(MeanSquare)) + (_State._HasRMSPlus3 ? dBCorrection : 0.0);
            m.NormalizedRMS = SmoothValue(NormalizeValue(m.RMS), m.NormalizedRMS);

            m.RMSTotal = 0.;
        }

        // Converts a signed level difference into the normalized meter range. Silence, denormalized input, and non-finite values produce the centered normalized value instead of NaN.
        const auto CalculateNormalizedDifference = [&](double Positive, double Negative) noexcept -> double
        {
            const double Denominator = std::max(Positive, Negative);

            if (!(Denominator > 0.) || !std::isfinite(Denominator))
                return NormalizeLLevelValue(0.);

            const double Difference = (Positive - Negative) / Denominator;

            if (!std::isfinite(Difference))
                return NormalizeLLevelValue(0.);

            return NormalizeLLevelValue(Difference);
        };

        {
            const double LeftRMS  = std::sqrt(_LeftSumSquared  / RMSFrameCount);
            const double RightRMS = std::sqrt(_RightSumSquared / RMSFrameCount);

            if (!std::isfinite(_Balance))
                _Balance = 0.5;

            _Balance = SmoothValue(CalculateNormalizedDifference(RightRMS, LeftRMS), _Balance);
        }

        {
            const double MidRMS  = std::sqrt(_MidSumSquared  / RMSFrameCount);
            const double SideRMS = std::sqrt(_SideSumSquared / RMSFrameCount);

            if (!std::isfinite(_Phase))
                _Phase = 0.5;

            _Phase = SmoothValue(CalculateNormalizedDifference(MidRMS, SideRMS), _Phase);
        }

        ResetRMSDependentValues();
    }

    /// <summary>
    /// Updates the peak values.
    /// </summary>
#ifdef v1
    void processor_t::AnimatePeaks(bool isStopped) noexcept
    {
        assert(_Options);

        const double DeltaTime = _Chrono.Elapsed();

        _Chrono.Reset();

        if (_State._VisualizationType == VisualizationType::PeakMeter)
        {
            const double AmplitudeRange = _Options->_AmplitudeHi - _Options->_AmplitudeLo;

            const double HoldTime = _State._HoldDuration * (double) _State._TargetRefreshRate;
            const double FallRate = (_State._FallSpeed != 0) ? msc::Map(DeltaTime, 0., AmplitudeRange / _State._FallSpeed, 0., 1.) : 0.;

            const bool IsAIMP = (_State._PeakMode == PeakMode::AIMP) || (_State._PeakMode == PeakMode::FadingAIMP);

            // Animate the smoothed peak and RMS values.
            for (auto & m : _Measurements)
            {
                if (m.NormalizedPeak >= m.MaxNormalizedPeak)
                {
                    if (IsAIMP)
                        m.HoldTime += (m.NormalizedPeak - m.MaxNormalizedPeak) * HoldTime;
                    else
                        m.HoldTime = HoldTime;

                    m.MaxNormalizedPeak = m.NormalizedPeak;
                    m.FallRate          = 0.;
                    m.Opacity           = 1.;
                }
                else
                {
                    if (m.HoldTime > 0.)
                    {
                        if (IsAIMP)
                            m.MaxNormalizedPeak += (m.HoldTime - std::max(m.HoldTime - 1., 0.)) / HoldTime;

                        m.HoldTime--;

                        if (IsAIMP)
                            m.HoldTime = std::min(m.HoldTime, HoldTime);
                    }
                    else
                    {
                        switch (_State._PeakMode)
                        {
                            default:

                            case PeakMode::None:
                                break;

                            case PeakMode::Classic:
                            {
                                m.FallRate           = FallRate;
                                m.MaxNormalizedPeak -= m.FallRate;
                                break;
                            }

                            case PeakMode::Gravity:
                            {
                                m.FallRate          += FallRate;
                                m.MaxNormalizedPeak -= m.FallRate;
                                break;
                            }

                            case PeakMode::FadeOut:
                            {
                                m.FallRate += FallRate;

                                m.Opacity -= m.FallRate;

                                if (m.Opacity <= 0.)
                                    m.MaxNormalizedPeak = m.NormalizedPeak;
                                break;
                            }

                            case PeakMode::AIMP:
                            {
                                m.FallRate           = FallRate * (1. + (int) (m.MaxNormalizedPeak < 0.5));
                                m.MaxNormalizedPeak -= m.FallRate;
                                break;
                            }

                            case PeakMode::FadingAIMP:
                            {
                                m.FallRate           = FallRate * (1. + (int) (m.MaxNormalizedPeak < 0.5));
                                m.MaxNormalizedPeak -= m.FallRate;

                                m.Opacity -= m.FallRate;

                                if (m.Opacity <= 0.)
                                    m.MaxNormalizedPeak = m.NormalizedPeak;
                                break;
                            }
                        }
                    }
                }
            }
        }
        else
        if ((_State._VisualizationType == VisualizationType::BalanceMeter) && isStopped)
        {
            const double Delta = 0.005;

            if (_Balance > 0.5)
                _Balance = std::clamp(_Balance - Delta, 0.5, 1.0);
            else
            if (_Balance < 0.5)
                _Balance = std::clamp(_Balance + Delta, 0.0, 0.5);

            if (_Phase > 0.5)
                _Phase = std::clamp(_Phase - Delta, 0.5, 1.0);
            else
            if (_Phase < 0.5)
                _Phase = std::clamp(_Phase + Delta, 0.0, 0.5);
        }
    }
#else
    void processor_t::AnimatePeaks(bool isStopped) noexcept
    {
        assert(_Options);

        const double DeltaTime = _Chrono.Elapsed();

        _Chrono.Reset();

        constexpr double GravityRampTime = 0.125; // In seconds

        const double AmplitudeRange      = _Options->_AmplitudeHi - _Options->_AmplitudeLo;  // In dB
        const double HoldDuration        = std::max(_State._HoldDuration, 0.);               // In seconds
        const double FallSpeed           = std::max(_State._FallSpeed, 0.) / AmplitudeRange; // In normalized units per second
        const double GravityAcceleration = FallSpeed / GravityRampTime;                      // In normalized units per second squared

        if (_State._VisualizationType == VisualizationType::PeakMeter)
        {
            const bool IsAIMP = (_State._PeakMode == PeakMode::AIMP) || (_State._PeakMode == PeakMode::FadingAIMP);

            for (auto & m : _Measurements)
            {
                if (m.NormalizedPeak > m.MaxNormalizedPeak)
                {
                    // Initialize a new peak.
                    const double PeakDifference = m.NormalizedPeak - m.MaxNormalizedPeak;

                    if (IsAIMP)
                        m.HoldDuration = std::min(m.HoldDuration + PeakDifference * HoldDuration, HoldDuration);
                    else
                        m.HoldDuration = HoldDuration;

                    m.MaxNormalizedPeak = m.NormalizedPeak;
                    m.FallSpeed         = 0.;
                    m.Opacity           = 1.;

                    continue;
                }

                double RemainingTime = DeltaTime;

                // Process the hold period in seconds.
                if (m.HoldDuration > 0.)
                {
                    const double HeldTime = std::min(m.HoldDuration, RemainingTime);

                    if (IsAIMP && (HoldDuration > 0.))
                        m.MaxNormalizedPeak += HeldTime / HoldDuration;

                    m.HoldDuration -= HeldTime;
                    RemainingTime  -= HeldTime;
                }

                if ((RemainingTime > 0.) && (m.HoldDuration <= 0.))
                {
                    switch (_State._PeakMode)
                    {
                        default:
                        case PeakMode::None:
                        {
                            m.MaxNormalizedPeak = m.NormalizedPeak;
                            m.HoldDuration          = 0.;
                            m.FallSpeed          = 0.;
                            m.Opacity           = 0.;
                            break;
                        }

                        case PeakMode::Classic:
                        {
                            m.FallSpeed = FallSpeed;

                            m.MaxNormalizedPeak -= m.FallSpeed * RemainingTime;

                            break;
                        }

                        case PeakMode::Gravity:
                        {
                            // Distance while accelerating: d = v0 * t + 0.5 *a * t^2
                            m.MaxNormalizedPeak -= (m.FallSpeed * RemainingTime) + (GravityAcceleration * RemainingTime * RemainingTime / 2.);
                            m.FallSpeed         += GravityAcceleration * RemainingTime;
                            break;
                        }

                        // The peak marker remains stationary and fades.
                        case PeakMode::FadeOut:
                        {
                            m.Opacity -= FallSpeed * RemainingTime;

                            if (m.Opacity <= 0.)
                            {
                                m.MaxNormalizedPeak = m.NormalizedPeak;
                                m.FallSpeed         = 0.;
                                m.Opacity           = 0.;
                            }

                            break;
                        }

                        case PeakMode::AIMP:
                        {
                            const double SpeedMultiplier = (m.MaxNormalizedPeak < 0.5) ? 2. : 1.;

                            m.FallSpeed = FallSpeed * SpeedMultiplier;

                            const double Distance = m.FallSpeed * RemainingTime;

                            m.MaxNormalizedPeak -= Distance;
                            break;
                        }

                        case PeakMode::FadingAIMP:
                        {
                            const double SpeedMultiplier = (m.MaxNormalizedPeak < 0.5) ? 2. : 1.;

                            m.FallSpeed = FallSpeed * SpeedMultiplier;

                            const double Distance = m.FallSpeed * RemainingTime;

                            m.MaxNormalizedPeak -= Distance;
                            m.Opacity           -= Distance;

                            if (m.Opacity <= 0.)
                            {
                                m.MaxNormalizedPeak = m.NormalizedPeak;
                                m.FallSpeed         = 0.;
                                m.Opacity           = 0.;
                            }

                            break;
                        }
                    }
                }

                m.MaxNormalizedPeak = std::clamp(m.MaxNormalizedPeak, m.NormalizedPeak, 1.);
                m.Opacity           = std::clamp(m.Opacity, 0., 1.);
            }
        }
        else
        if ((_State._VisualizationType == VisualizationType::BalanceMeter) && isStopped)
        {
            const double Distance = FallSpeed * DeltaTime;

            if (_Balance > 0.5)
                _Balance = std::max(_Balance - Distance, 0.5);
            else
            if (_Balance < 0.5)
                _Balance = std::min(_Balance + Distance, 0.5);

            if (_Phase > 0.5)
                _Phase = std::max(_Phase - Distance, 0.5);
            else
            if (_Phase < 0.5)
                _Phase = std::min(_Phase + Distance, 0.5);
        }
    }
#endif

    /// <summary>
    /// Initializes the peak measurements before processing an audio chunk.
    /// </summary>
    void processor_t::InitializeMeasurements(uint32_t selectedChannels) noexcept
    {
        if (_SelectedChannels != selectedChannels)
        {
            // The chunk configuration has changed. Recreate the measurements.
            static constexpr const WCHAR * const ChannelNames[] =
            {
                L"FL", L"FR", L"FC",
                L"LFE",
                L"BL", L"BR", 
                L"FCL", L"FCR",
                L"BC", L"SL", L"SR", L"TC",
                L"TFL", L"TFC", L"TFR", L"TBL", L"TBC", L"TBR",
            };

            size_t i = 0;

            _Measurements.clear();

            for (uint32_t ActiveChannelMask = selectedChannels; (ActiveChannelMask != 0) && (i < _countof(ChannelNames)); ActiveChannelMask >>= 1, ++i)
            {
                if (ActiveChannelMask & 1)
                    _Measurements.push_back({ (Channels)(1 << i), ChannelNames[i] });
            }

            _SelectedChannels = selectedChannels;

            ResetRMSDependentValues();
        }
        else
        {
            // Reset only the peak level of each bar to -∞.
            for (auto & m : _Measurements)
                m.Peak = -std::numeric_limits<double>::infinity();
        }
    }

    /// <summary>
    /// Resets the measurements.
    /// </summary>
    void processor_t::ResetMeasurements(ResetOptions options) noexcept
    {
        if (!IsSet(options, ResetOptions::Value))
            return;

        for (auto & m : _Measurements)
        {
            m.Peak =
            m.RMS  = -std::numeric_limits<double>::infinity();

            m.NormalizedPeak =
            m.NormalizedRMS  = 0.;
        }
    }

    /// <summary>
    /// Resets the RMS window dependent values.
    /// </summary>
    void processor_t::ResetRMSDependentValues() noexcept
    {
        _RMSTimeElapsed = 0.;
        _RMSFrameCount = 0;

        for (auto & m : _Measurements)
            m.RMSTotal = 0.;

        _LeftSumSquared  = 0.;
        _RightSumSquared = 0.;

        _MidSumSquared   = 0.;
        _SideSumSquared  = 0.;
    }
}


/** $VER: MeteringProcessor.cpp (2026.10.28) P. Stuer - Implements an audio processor for the metering visualization. **/

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
            ResetPeakMeasurements();
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

        struct meter_channel_t
        {
            uint32_t SampleOffset {};
            size_t MeasurementIndex {};
        };

        std::array<meter_channel_t, MaximumChannelCount> MeterChannels {};

        size_t MeterChannelCount = 0;

        std::array<uint32_t, BalanceChannelCount> BalanceOffsets {};

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

                    if (IsActive && (MeasurementIndex < _PeakMeasurements.size()))
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

        for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
        {
            const audio_sample * const Frame = Frames + (FrameIndex * (size_t) ChannelCount);

            for (size_t ChannelIndex = 0; ChannelIndex < MeterChannelCount; ++ChannelIndex)
            {
                const meter_channel_t & Channel = MeterChannels[ChannelIndex];

                const double Sample    = (double) Frame[Channel.SampleOffset];
                const double Magnitude = std::abs(Sample);

                auto & Measurement = _PeakMeasurements[Channel.MeasurementIndex];

                Measurement.Peak      = std::max(Measurement.Peak, Magnitude);
                Measurement.RMSTotal += Sample * Sample;
            }

            // Reset the balance samples for every frame. This prevents a sample from an earlier frame being reused when fewer than two paired channels are available.
            std::array<double, BalanceChannelCount> BalanceSamples {};

            for (size_t ChannelIndex = 0; ChannelIndex < BalanceOffsetCount; ++ChannelIndex)
                BalanceSamples[ChannelIndex] = (double) Frame[BalanceOffsets[ChannelIndex]];

            const double Left  = BalanceSamples[0];
            const double Right = BalanceSamples[1];

            _Left  += Left  * Left;
            _Right += Right * Right;

            const double Mid  = (Left + Right) * 0.5;
            const double Side = (Left - Right) * 0.5;

            _Mid  += Mid  * Mid;
            _Side += Side * Side;
        }

        _RMSFrameCount  += FrameCount;
        _RMSTimeElapsed += chunk.get_duration();

        // Normalize and smooth the peak values. https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
        for (auto & Measurement : _PeakMeasurements)
        {
            Measurement.Peak           = ToDecibel(Measurement.Peak);
            Measurement.NormalizedPeak = SmoothValue(NormalizeValue(Measurement.Peak), Measurement.NormalizedPeak);
        }

        // Keep accumulating until the configured RMS window has elapsed.
        if (_RMSTimeElapsed < _State._RMSWindow)
            return;

        const double RMSFrameCount = (double) _RMSFrameCount;

        for (auto & Measurement : _PeakMeasurements)
        {
            const double MeanSquare = Measurement.RMSTotal / RMSFrameCount;

            // https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
            Measurement.RMS           = ToDecibel(std::sqrt(MeanSquare)) + (_State._HasRMSPlus3 ? dBCorrection : 0.0);
            Measurement.NormalizedRMS = SmoothValue(NormalizeValue(Measurement.RMS), Measurement.NormalizedRMS);

            Measurement.RMSTotal = 0.0;
        }

        const double LeftRMS  = std::sqrt(_Left  / RMSFrameCount);
        const double RightRMS = std::sqrt(_Right / RMSFrameCount);

        // Converts a signed level difference into the normalized meter range. Silence, denormalized input, and non-finite values produce the centered normalized value instead of NaN.
        const auto CalculateNormalizedDifference = [&](double Positive, double Negative) noexcept -> double
        {
                const double Denominator = std::max(Positive, Negative);

                if (!(Denominator > 0.0) || !std::isfinite(Denominator))
                    return NormalizeLLevelValue(0.0);

                const double Difference = (Positive - Negative) / Denominator;

                if (!std::isfinite(Difference))
                    return NormalizeLLevelValue(0.0);

                return NormalizeLLevelValue(Difference);
        };

        if (!std::isfinite(_Balance))
            _Balance = 0.5;

        _Balance = SmoothValue(CalculateNormalizedDifference(RightRMS, LeftRMS), _Balance);

        const double MidRMS  = std::sqrt(_Mid  / RMSFrameCount);
        const double SideRMS = std::sqrt(_Side / RMSFrameCount);

        if (!std::isfinite(_Phase))
            _Phase = 0.5;

        _Phase = SmoothValue(CalculateNormalizedDifference(MidRMS, SideRMS), _Phase);

        ResetRMSDependentValues();
    }

    /// <summary>
    /// Updates the peak values.
    /// </summary>
    void processor_t::UpdatePeaks(bool isStopped) noexcept
    {
        assert(_Options);

        const double Elapsed = _Chrono.Elapsed();
        const double AmplitudeRange = _Options->_AmplitudeHi - _Options->_AmplitudeLo;

        const double HoldTime = _State._HoldTime * (double) _State._TargetRefreshRate;
        const double FallRate = (_State._FallRate != 0) ? msc::Map(Elapsed, 0., AmplitudeRange / _State._FallRate, 0., 1.) : 0.;

        const bool IsAIMP = (_State._PeakMode == PeakMode::AIMP) || (_State._PeakMode == PeakMode::FadingAIMP);

        if (_State._VisualizationType == VisualizationType::PeakMeter)
        {
            // Animate the smoothed peak and RMS values.
            for (auto & m : _PeakMeasurements)
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
        if (_State._VisualizationType == VisualizationType::LevelMeter)
        {
            if (isStopped)
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

        _Chrono.Reset();
    }

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

            _PeakMeasurements.clear();

            for (uint32_t ActiveChannelMask = selectedChannels; (ActiveChannelMask != 0) && (i < _countof(ChannelNames)); ActiveChannelMask >>= 1, ++i)
            {
                if (ActiveChannelMask & 1)
                    _PeakMeasurements.push_back({ (Channels)(1 << i), ChannelNames[i] });
            }

            _SelectedChannels = selectedChannels;

            ResetRMSDependentValues();
        }
        else
        {
            // Reset only the peak level of each bar to -∞.
            for (auto & m : _PeakMeasurements)
                m.Peak = -std::numeric_limits<double>::infinity();
        }
    }

    /// <summary>
    /// Resets the peak measurements.
    /// </summary>
    void processor_t::ResetPeakMeasurements() noexcept
    {
        for (peak_measurement_t & m : _PeakMeasurements)
        {
            m.Peak = m.RMS = -std::numeric_limits<double>::infinity();
            m.NormalizedPeak = m.NormalizedRMS = 0.;
        }
    }

    /// <summary>
    /// Resets the RMS window dependent values.
    /// </summary>
    void processor_t::ResetRMSDependentValues() noexcept
    {
        _RMSTimeElapsed = 0.;
        _RMSFrameCount = 0;

        _Left  = 0.;
        _Right = 0.;

        _Mid   = 0.;
        _Side  = 0.;
    }
}

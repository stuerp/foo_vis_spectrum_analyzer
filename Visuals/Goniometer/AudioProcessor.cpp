
/** $VER: AudioProcessor.cpp (2026.09.21) P. Stuer - Implements an audio processor for the goniometer. **/

#include <pch.h>

#include "AudioProcessor.h"

#include <cmath>
#include <numbers>

/// <summary>
/// Configures the audio processor.
/// </summary>
HRESULT audio_processor_t::Configure(double lowBand, double highBand, double sampleRate) noexcept
{
    if ((lowBand >= highBand) || (sampleRate <= 0.))
        return E_INVALIDARG;

    if ((lowBand == _LowBand) && (highBand == _HighBand) && (sampleRate == _SampleRate))
        return S_FALSE;

    _LowBand    = lowBand;
    _HighBand   = highBand;
    _SampleRate = sampleRate;

    HRESULT hr = _CrossoverL.Configure(_LowBand, _HighBand, sampleRate);

    if (FAILED(hr))
        return hr;

    hr = _CrossoverR.Configure(_LowBand, _HighBand, sampleRate);

    if (FAILED(hr))
        return hr;

    hr = _CrossoverL.SetCrossoverMode(_CrossoverMode);

    if (FAILED(hr))
        return hr;

    hr = _CrossoverR.SetCrossoverMode(_CrossoverMode);

    return hr;
}

/// <summary>
/// 
/// </summary>
HRESULT audio_processor_t::SetCrossoverMode(crossover_filter_t::Mode mode) noexcept
{
    if (mode == _CrossoverMode)
        return S_FALSE;

    _CrossoverMode = mode;

    HRESULT hr = _CrossoverL.SetCrossoverMode(_CrossoverMode);

    if (FAILED(hr))
        return hr;

    hr = _CrossoverR.SetCrossoverMode(_CrossoverMode);

    return hr;
}

/// <summary>
/// Processes the audio frames.
/// </summary>
void audio_processor_t::Process(const audio_chunk_impl & chunk, uint32_t activeChannelMask, uint32_t pairedChannelMask, double lowBand, double highBand) noexcept
{
    _PointCount = 0;

    Configure(lowBand, highBand, (double) chunk.get_sample_rate());

    const size_t FrameCount             = chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount         = chunk.get_channel_count();
    const uint32_t AvailableChannelMask = chunk.get_channel_config();  // Mask containing the channels in the audio chunk.

    const uint32_t ChannelMask = AvailableChannelMask & activeChannelMask & ((ChannelCount > 1) ? pairedChannelMask : ~0u);

    if ((FrameCount == 0) || (ChannelCount < 1) || (ChannelMask == 0))
        return;

    size_t ChannelIndexL = 0;
    size_t ChannelIndexR = 0;

    if (ChannelCount > 1)
    {
        ChannelIndexL = (size_t)       std::countr_zero(ChannelMask);
        ChannelIndexR = (size_t) (31 - std::countl_zero(ChannelMask));
    }

    {
        const auto PointCapacity = (_ColorMode == ColorMode::Triband) ? (FrameCount * 3) : FrameCount;

        if (_Points.size() < PointCapacity)
            _Points.resize(PointCapacity);
    }

    double LeftPowerSum = 0., RightPowerSum = 0., CrossPowerSum = 0.;

    const audio_sample * CurrentFrame = chunk.get_data();

    for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
    {
        const auto SampleL = CurrentFrame[ChannelIndexL];
        const auto SampleR = CurrentFrame[ChannelIndexR];

        {
            // Reset the low-band, mid-band, and high-band amplitudes of both channels.
            double LowL = 0., MidL = 0., HighL = 0., LowR = 0., MidR = 0., HighR = 0.;

            _CrossoverL.Process(SampleL, LowL, MidL, HighL);
            _CrossoverR.Process(SampleR, LowR, MidR, HighR);

            if (_ColorMode == ColorMode::Mono)
            {
                AddPoint(SampleL, SampleR, _MonoColor, _MidVisualGain);
            }
            else
            {
                const double LowBandPower  = (LowL  * LowL)  + (LowR  * LowR);
                const double MidBandPower  = (MidL  * MidL)  + (MidR  * MidR);
                const double HighBandPower = (HighL * HighL) + (HighR * HighR);

                if (_ColorMode == ColorMode::RGB)
                {
                    const auto TotalBandPower = LowBandPower + MidBandPower + HighBandPower + 1e-12;

                    const auto Color = D2D1::ColorF((float) std::sqrt(LowBandPower / TotalBandPower), (float) std::sqrt(MidBandPower / TotalBandPower), (float) std::sqrt(HighBandPower / TotalBandPower));

                    AddPoint(SampleL, SampleR, Color, _MidVisualGain);
                }
                else
                {
                    if (LowBandPower > _BandPowerThreshold)
                        AddPoint(LowL,  LowR,  _LowColor,  _LowVisualGain);

                    if (MidBandPower > _BandPowerThreshold)
                        AddPoint(MidL,  MidR,  _MidColor,  _MidVisualGain);

                    if (HighBandPower > _BandPowerThreshold)
                        AddPoint(HighL, HighR, _HighColor, _HighVisualGain);
                }
            }
        }

        // Update the correlation accumulators.
        LeftPowerSum  += SampleL * SampleL;
        RightPowerSum += SampleR * SampleR;
        CrossPowerSum += SampleL * SampleR;

        CurrentFrame += ChannelCount;
    }

    // Calculate the normalized cross-correlation coefficient.
    {
        const double Normalizer = std::sqrt(LeftPowerSum * RightPowerSum);

        const double InstantCorrelation = (Normalizer > 1e-20) ? std::clamp(CrossPowerSum / Normalizer, -1., 1.) : 0.;

        _Correlation += (InstantCorrelation - _Correlation) * .16; // Avoid jitter by smoothing the value.
    }
}

/// <summary>
/// Adds a point.
/// </summary>
void audio_processor_t::AddPoint(double l, double r, const D2D1_COLOR_F color, double opacity) noexcept
{
    constexpr double InverseSqrt2 = 1. / std::numbers::sqrt2_v<double>; // = cos 45 = sin 45

    const auto x = (float) std::clamp((l - r) * InverseSqrt2, -1., 1.); // Side component
    const auto y = (float) std::clamp((l + r) * InverseSqrt2, -1., 1.); // Mid component

    _Points[_PointCount++] =
    {
        x, y,
        D2D1::ColorF(color.r, color.g, color.b, (float) opacity),
    };
}

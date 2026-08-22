
/** $VER: Decimator.cpp (2026.08.22) P. Stuer - Implements a decimator **/

#include "pch.h"

#include "Decimator.h"

/// <summary>
/// Decimates the samples of a chunk to another taking into account the specified ratio.
/// </summary>
void decimator_t::Process(const audio_chunk & srcChunk, audio_chunk & dstChunk, double ratio) noexcept
{
    const size_t ChannelCount  = srcChunk.get_channels();
    const size_t SrcFrameCount = srcChunk.get_sample_count();

    const audio_sample * const __restrict SrcFrames = srcChunk.get_data();

    if ((ChannelCount == 0) || (SrcFrameCount == 0) || (ratio <= 0.0) || (SrcFrames == nullptr))
    {
        dstChunk.set_sample_count(0);
        dstChunk.set_data_size(0);

        return;
    }

    const size_t DstFrameCount = std::max<size_t>(1, (size_t) std::ceil((double) SrcFrameCount / ratio));

    if (DstFrameCount >= SrcFrameCount)
    {
        dstChunk.copy(srcChunk, true);

        return;
    }

    dstChunk.set_channels((uint32_t) ChannelCount, srcChunk.get_channel_config());
    dstChunk.set_sample_rate(std::max<uint32_t>(1, (uint32_t)(std::lround(srcChunk.get_sample_rate() / ratio))));
    dstChunk.set_sample_count(DstFrameCount);
    dstChunk.set_data_size(DstFrameCount * ChannelCount);

    audio_sample * const __restrict DstFrames = dstChunk.get_data();

    // Peak envelope aggregation
    #pragma loop(hint_parallel(8))
    for (size_t DstFrameIndex = 0; DstFrameIndex < DstFrameCount; ++DstFrameIndex)
    {
        const size_t SrcBeginIndex = (DstFrameIndex * SrcFrameCount) / DstFrameCount;
        const size_t SrcEndIndex   = std::min(SrcFrameCount, std::max(SrcBeginIndex + 1, ((DstFrameIndex + 1) * SrcFrameCount) / DstFrameCount));

        audio_sample * __restrict Dst = &DstFrames[DstFrameIndex * ChannelCount];

        for (size_t ch = 0; ch < ChannelCount; ++ch)
        {
            size_t SrcFrameIndex = SrcBeginIndex;

            // Retain the largest magnitude of this source sub-chunk.
            double Peak = SrcFrames[(SrcFrameIndex++ * ChannelCount) + ch];
            double PeakMagnitude = std::abs(Peak);

            while (SrcFrameIndex < SrcEndIndex)
            {
                const double Sample = SrcFrames[(SrcFrameIndex++ * ChannelCount) + ch];
                const double Magnitude = std::abs(Sample);

                if (Magnitude > PeakMagnitude)
                {
                    Peak = Sample;
                    PeakMagnitude = Magnitude;
                }
            }

            *Dst++ = (audio_sample) Peak;
        }
    }
}

/// <summary>
/// Decimates the samples of a chunk to another taking into account the specified ratio using filtering.
/// </summary>
void decimator_t::ProcessFiltered(const audio_chunk & srcChunk, audio_chunk & dstChunk, double ratio) noexcept
{
    const uint32_t ChannelCount = srcChunk.get_channels();

    const uint32_t SrcSampleRate = srcChunk.get_sample_rate();
    const size_t SrcFrameCount   = srcChunk.get_sample_count();

    const audio_sample * __restrict SrcFrames = srcChunk.get_data();

    if ((ChannelCount == 0) || (SrcFrameCount == 0) || (ratio <= 0.0) || (SrcFrames == nullptr))
    {
        dstChunk.set_sample_count(0);
        dstChunk.set_data_size(0);

        return;
    }

    const auto DstSampleRate = (uint32_t) std::round(SrcSampleRate / ratio);

    if ((DstSampleRate == 0) || (std::abs((int) DstSampleRate - (int) SrcSampleRate) < 1))
    {
        dstChunk.copy(srcChunk, true);

        return;
    }

    const auto DstFrameCount = (size_t) std::ceil((double) SrcFrameCount / ratio);

    dstChunk.set_channels(ChannelCount, srcChunk.get_channel_config());
    dstChunk.set_sample_rate(DstSampleRate);
    dstChunk.set_sample_count(DstFrameCount);
    dstChunk.set_data_size(DstFrameCount * ChannelCount);

    audio_sample * __restrict DstFrames = dstChunk.get_data();

    if (_FilteredFrames.capacity() != SrcFrameCount * ChannelCount)
        _FilteredFrames.resize(SrcFrameCount * ChannelCount);

    const auto & Stride = ChannelCount;

    // Apply an anti-aliasing low-pass filter.
    {
        std::vector<double> FilterState(ChannelCount, 0.);

        const double Alpha = 0.85 * (0.45 / ratio); // Conservative cutoff

        for (size_t ch = 0; ch < ChannelCount; ++ch)
        {
            double State = 0.;

            for (size_t i = 0; i < SrcFrameCount; ++i)
            {
                const double Sample = SrcFrames[(i * Stride) + ch];

                State += (Sample - State) * Alpha;

                _FilteredFrames[(i * Stride) + ch] = (audio_sample) State;
            }

            FilterState[ch] = State;
        }
    }

    // Apply linear interpolation decimation on the filtered samples.
    for (uint32_t ch = 0; ch < ChannelCount; ++ch)
    {
        size_t DstIndex = ch;

        for (size_t DstFrameIndex = 0; DstFrameIndex < DstFrameCount; ++DstFrameIndex)
        {
            const double SrcFrameIndex = std::min((double) DstFrameIndex * ratio, (double) (SrcFrameCount - 1));

            const size_t Index = (size_t) SrcFrameIndex;
            const double Fraction = SrcFrameIndex - (double) Index;

            if (Index + 1 < SrcFrameCount)
            {
                const audio_sample s0 = _FilteredFrames[( Index      * Stride) + ch];
                const audio_sample s1 = _FilteredFrames[((Index + 1) * Stride) + ch];

                DstFrames[DstIndex] = s0 * (audio_sample) (1. - Fraction) + (s1 * (audio_sample) Fraction);
            }
            else
                DstFrames[DstIndex] = _FilteredFrames[(Index * Stride) + ch];

            DstIndex += Stride;
        }
    }
}

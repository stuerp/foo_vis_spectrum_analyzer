
/** $VER: Downsampler.cpp (2026.08.22) P. Stuer - Implements a downsampler **/

#include "pch.h"

#include "Downsampler.h"

/// <summary>
/// Downsamples an audio chunk taking into account the specified ratio.
/// </summary>
void downsampler_t::Process(const audio_chunk & srcChunk, audio_chunk & dstChunk, double ratio) noexcept
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

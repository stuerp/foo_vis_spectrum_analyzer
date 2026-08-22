
/** $VER: Downmixer.cpp (2026.08.22) P. Stuer - Implements a downmixer to mono **/

#include "pch.h"

#include "Downmixer.h"

/// <summary>
/// Downmixes an audio chunk to a mono audio chunk.
/// </summary>
void downmixer_t::operator()(const audio_chunk & srcChunk, uint32_t selectedChannels, audio_chunk & dstChunk) const noexcept
{
    const uint32_t ChannelCount  = srcChunk.get_channels();
    const size_t FrameCount = srcChunk.get_sample_count();

    dstChunk.set_channels(1, audio_chunk::channel_config_mono);
    dstChunk.set_sample_rate(srcChunk.get_sample_rate());
    dstChunk.set_sample_count(FrameCount);
    dstChunk.set_data_size(FrameCount);

    const audio_sample * __restrict Src = srcChunk.get_data();
    audio_sample * __restrict Dst = dstChunk.get_data();

    for (size_t i = 0; i < FrameCount; ++i, Src += ChannelCount)
        *Dst++ = operator()(Src, ChannelCount, selectedChannels);
}

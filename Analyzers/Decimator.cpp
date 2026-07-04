
/** $VER: Decimator.cpp (2026.07.04) P. Stuer - Implements a decimator **/

#include "pch.h"

#include "Decimator.h"

void decimator_t::Process(const audio_chunk & srcChunk, audio_chunk & dstChunk, double ratio) noexcept
{
    const uint32_t ChannelCount = srcChunk.get_channels();

    const uint32_t SrcSampleRate   = srcChunk.get_sample_rate();
    const size_t SrcFrameCount     = srcChunk.get_sample_count();

    const audio_sample * __restrict SrcFrames = srcChunk.get_data();

    const uint32_t DstSampleRate = (uint32_t) std::round(SrcSampleRate / ratio);

    if (DstSampleRate == 0 || std::abs((int) DstSampleRate - (int) SrcSampleRate) < 1)
    {
        dstChunk.set_data(SrcFrames, SrcFrameCount, ChannelCount, SrcSampleRate);

        return;
    }

    const size_t DstFrameCount = (size_t) std::ceil((double) SrcFrameCount / ratio);

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

        for (uint32_t ch = 0; ch < ChannelCount; ++ch)
        {
            double State = 0.;

            for (size_t i = 0; i < SrcFrameCount; ++i)
            {
                double Sample = SrcFrames[(i * Stride) + ch];

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


/** $VER: Downmixer.h (2026.08.22) P. Stuer - Implements a downmix-to-mono scaler as a functor. **/

#pragma once

/// <summary>
/// Implements a downmix-to-mono scaler as a functor.
/// </summary>
class downmixer_t
{
public:
    explicit downmixer_t() noexcept { }

    /// <summary>
    /// Downmixes an audio frame to a mono sample.
    /// </summary>
    inline audio_sample operator()(const audio_sample * frame, uint32_t channelCount, uint32_t selectedChannels) const noexcept
    {
        if ((frame == nullptr) || (selectedChannels == 0))
            return (audio_sample) 0.;

        double Sample = 0.;
        uint32_t n = 0;

        uint32_t SelectedChannels = selectedChannels;

        for (uint32_t i = 0; (i < channelCount) && (SelectedChannels != 0); ++i, ++frame, SelectedChannels >>= 1)
        {
            if (SelectedChannels & 1)
            {
                Sample += (double) *frame;
                n++;
            }
        }

        return (n > 0) ? (audio_sample) (Sample / n) : (audio_sample) 0.;
    }

    void operator()(const audio_chunk & srcChunk, uint32_t selectedChannels, audio_chunk & dstChunk) const noexcept;
};

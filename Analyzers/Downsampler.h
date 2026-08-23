
/** $VER: Downsampler.h (2026.08.22) P. Stuer - Implements a downsampler **/

#pragma once

/// <summary>
/// Implements a downsampler.
/// </summary>
class downsampler_t
{
public:
    void Process(const audio_chunk & srcChunk, audio_chunk & dstChunk, double ratio = 2.0) noexcept;
};

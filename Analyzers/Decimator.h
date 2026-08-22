
/** $VER: Decimator.h (2026.08.22) P. Stuer - Implements a decimator **/

#pragma once

/// <summary>
/// Implements a decimator.
/// </summary>
class decimator_t
{
public:
    void ProcessFiltered(const audio_chunk & srcChunk, audio_chunk & dstChunk, double ratio = 2.0) noexcept;
    void Process(const audio_chunk & srcChunk, audio_chunk & dstChunk, double ratio = 2.0) noexcept;

private:
    std::vector<audio_sample> _FilteredFrames;
};

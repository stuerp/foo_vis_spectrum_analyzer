
/** $VER: OscilloscopeProcessor.h (2026.10.28) P. Stuer - Implements an audio processor for the oscilloscope visualizations. **/

#pragma once

#include "AudioProcessor.h"
#include "State.h"

#include <sdk/audio_chunk.h>

namespace Oscilloscope
{

class processor_t final : public audio_processor_t
{
public:
    processor_t(state_t & state) : audio_processor_t(state) { }

    processor_t(const processor_t &) = delete;
    processor_t(processor_t &&) = delete;

    processor_t & operator=(const processor_t &) = delete;
    processor_t & operator=(processor_t &&) = delete;

    virtual ~processor_t() = default;

public:
    void Configure(const graph_options_t * options) noexcept override final;
    void Reset() noexcept override final;
    void Process(const audio_chunk & chunk) noexcept override final;
    void UpdatePeaks(bool isStopped) noexcept override final;

public:
    audio_chunk_impl _Chunk;
};

}

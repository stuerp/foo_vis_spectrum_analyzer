
/** $VER: TestProcessor.cpp (2026.10.28) P. Stuer - Implements an audio processor for the test visualization. **/

#include <pch.h>

#include "TestProcessor.h"

namespace Test
{
    void processor_t::Configure(const graph_options_t * options) noexcept
    {
        __super::Configure(options);
    }

    void processor_t::Reset() noexcept
    {
        __super::Reset();
    }

    void processor_t::Process(const audio_chunk & chunk) noexcept
    {
        _Chunk.copy(chunk, true);
    }
}

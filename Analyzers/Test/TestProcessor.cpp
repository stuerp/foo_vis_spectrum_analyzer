
/** $VER: TestProcessor.cpp (2026.10.28) P. Stuer - Implements an audio processor for the test visualization. **/

#include <pch.h>

#include "TestProcessor.h"

namespace Test
{
    /// <summary>
    /// 
    /// </summary>
    void processor_t::Configure(const graph_options_t * options) noexcept
    {
        __super::Configure(options);
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::Reset() noexcept
    {
        __super::Reset();

        _Chunk.reset();
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::Process(const audio_chunk & chunk) noexcept
    {
        _Chunk.copy(chunk, true);
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::UpdatePeaks(bool isStopped) noexcept
    {
        if (isStopped)
            _Chunk.reset();
    }
}

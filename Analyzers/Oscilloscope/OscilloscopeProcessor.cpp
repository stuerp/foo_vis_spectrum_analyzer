
/** $VER: OscilloscopeProcessor.cpp (2026.10.28) P. Stuer - Implements an audio processor for the oscilloscope visualization. **/

#include <pch.h>

#include "OscilloscopeProcessor.h"

#include "Downmixer.h"

namespace Oscilloscope
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
        if (_State._Downmix)
        {
            downmixer_t Downmixer;

            Downmixer(chunk, _Options->_ActiveChannelMask, _Chunk);
        }
        else
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

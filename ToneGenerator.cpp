
/** $VER: ToneGenerator.cpp (2024.02.13) P. Stuer - Generates a waveform and can produce foobar2000-compliant audio chunks for testing purposes. **/

#include "pch.h"
#include "ToneGenerator.h"

#pragma hdrstop

/// <summary>
/// Gets a chunk of audio samples from the generator.
/// </summary>
bool tone_generator_t::GetChunk(audio_chunk & chunk, uint32_t sampleRate)
{
    for (size_t i = 0; i < _Frames.size(); ++i)
    {
        _Frames[i] = (audio_sample) (::sin(_Clock / (double) sampleRate * _Frequency * M_PI * 2.) * _Amplitude + (0.5 - (double) ::rand() / (double) RAND_MAX) * _NoiseAmplitude);
        _Clock++;
    }

    chunk.set_data(_Frames.data(), _Frames.size(), _ChannelCount, sampleRate);

    return true;
}

tone_generator_t _ToneGenerator;

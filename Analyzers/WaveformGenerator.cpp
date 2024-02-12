
/** $VER: WaveformGenerator.cpp (2024.02.12) P. Stuer - Generates a waveform and can produce foobar2000-compliant audio chunks for testing purposes. **/

#include "WaveformGenerator.h"

#pragma hdrstop

/// <summary>
/// Gets a chunk of audio samples from the generator.
/// </summary>
bool WaveformGenerator::GetChunk(audio_chunk & chunk, unsigned int sampleRate)
{
    for (size_t i = 0; i < _ChunkSize; i++)
    {
        _Data[i] = ::sin(_Clock / (double) sampleRate * _Frequency * M_PI * 2.) * _Amplitude + (0.5 - (double) ::rand() / (double) RAND_MAX) * _NoiseAmplitude;
        _Clock++;
    }

    chunk.set_data(_Data, _ChunkSize, 1, sampleRate);

    return true;
}

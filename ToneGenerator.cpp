
/** $VER: ToneGenerator.cpp (2024.02.13) P. Stuer - Generates a waveform and can produce foobar2000-compliant audio chunks for testing purposes. **/

#include "pch.h"
#include "ToneGenerator.h"

#pragma hdrstop

/// <summary>
/// Gets a chunk of audio samples from the generator.
/// </summary>
bool ToneGenerator::GetChunk(audio_chunk & chunk, uint32_t sampleRate)
{
    for (size_t i = 0; i < _Data.size(); ++i)
    {
        _Data[i] = (audio_sample) (::sin(_Clock / (double) sampleRate * _Frequency * M_PI * 2.) * _Amplitude + (0.5 - (double) ::rand() / (double) RAND_MAX) * _NoiseAmplitude);
        _Clock++;
    }

    chunk.set_data(_Data.data(), _Data.size(), _ChannelCount, sampleRate);

    return true;
}

ToneGenerator _ToneGenerator;

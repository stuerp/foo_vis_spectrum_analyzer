
/** $VER: WaveformGenerator.h (2024.02.12) P. Stuer - Generates a waveform and can produce foobar2000-compliant audio chunks for testing purposes. **/

#pragma once

#include "framework.h"

class WaveformGenerator
{
public:
    WaveformGenerator(double frequency, double amplitude, double noiseAmplitude, size_t chunkSize) : _Clock(), _Frequency(frequency), _Amplitude(amplitude), _NoiseAmplitude(noiseAmplitude), _ChunkSize(chunkSize)
    {
        _Data = new audio_sample[chunkSize];
    }

    virtual ~WaveformGenerator()
    {
        delete[] _Data;
    }

    bool GetChunk(audio_chunk & chunk, unsigned int sampleRate);

private:
    double _Clock;

    double _Frequency; // Hz
    double _Amplitude;
    double _NoiseAmplitude;

    size_t _ChunkSize;
    audio_sample * _Data;
};

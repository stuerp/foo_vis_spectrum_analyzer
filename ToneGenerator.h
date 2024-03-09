
/** $VER: ToneGenerator.h (2024.03.09) P. Stuer - Generates a waveform and can produce foobar2000-compliant audio chunks for testing purposes. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#define NOMINMAX
#include <helpers/foobar2000+atl.h>
#include <helpers/helpers.h>
#undef NOMINMAX

class ToneGenerator
{
public:
    ToneGenerator() : _Data(), _Size() { };

    void Initialize(double frequency, double amplitude, double noiseAmplitude, size_t bufferSize)
    {
        _Frequency = frequency;
        _Amplitude = amplitude;
        _NoiseAmplitude = noiseAmplitude;
        _Size = bufferSize;

        Reset();

        _Data = new audio_sample[_Size];
    }

    virtual ~ToneGenerator()
    {
        Reset();
    }

    bool GetChunk(audio_chunk & chunk, uint32_t sampleRate);

    void Reset()
    {
        _Clock = 0.;

        if (_Data)
        {
            delete[] _Data;
            _Data = nullptr;
        }
    }

private:
    double _Clock;

    double _Frequency; // Hz
    double _Amplitude;
    double _NoiseAmplitude;

    const uint32_t _ChannelCount = 1;

    audio_sample * _Data;
    size_t _Size;
};

extern ToneGenerator _ToneGenerator;

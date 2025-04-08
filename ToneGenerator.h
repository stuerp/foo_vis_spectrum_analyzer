
/** $VER: ToneGenerator.h (2025.04.08) P. Stuer - Generates a waveform and can produce foobar2000-compliant audio chunks for testing purposes. **/

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

#include <vector>

class ToneGenerator
{
public:
    ToneGenerator() { };

    void Initialize(double frequency, double amplitude, double noiseAmplitude, size_t bufferSize)
    {
        _Frequency = frequency;
        _Amplitude = amplitude;
        _NoiseAmplitude = noiseAmplitude;

        Reset();

        if (bufferSize != 0)
            _Data.resize(bufferSize);
    }

    virtual ~ToneGenerator()
    {
        Reset();
    }

    bool GetChunk(audio_chunk & chunk, uint32_t sampleRate);

    void Reset()
    {
        _Clock = 0.;

        _Data.clear();
    }

private:
    double _Clock;

    double _Frequency; // Hz
    double _Amplitude;
    double _NoiseAmplitude;

    const uint32_t _ChannelCount = 1;

    std::vector<audio_sample> _Data;
};

extern ToneGenerator _ToneGenerator;

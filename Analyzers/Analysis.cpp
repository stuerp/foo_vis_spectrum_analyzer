
/** $VER: Analysis.cpp (2024.02.15) P. Stuer **/

#include "Analysis.h"

#include "Support.h"

#pragma hdrstop

void Analysis::Initialize(const audio_chunk & chunk) noexcept
{
    if (_WindowFunction == nullptr)
        _WindowFunction = WindowFunction::Create(_State._WindowFunction, _State._WindowParameter, _State._WindowSkew, _State._Truncate);

    if (_BrownPucketteKernel == nullptr)
        _BrownPucketteKernel = WindowFunction::Create(_State._KernelShape, _State._KernelShapeParameter, _State._KernelAsymmetry, _State._Truncate);

    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t ChannelSetup = chunk.get_channel_config();

    if ((_FFTAnalyzer == nullptr) && (_State._Transform == Transform::FFT))
    {
        _FFTAnalyzer = new FFTAnalyzer(&_State, _State._SampleRate, ChannelCount, ChannelSetup, *_WindowFunction, *_BrownPucketteKernel, _State._NumBins);
    }

    if ((_CQTAnalyzer == nullptr) && (_State._Transform == Transform::CQT))
    {
        _CQTAnalyzer = new CQTAnalyzer(&_State, _State._SampleRate, ChannelCount, ChannelSetup, *_WindowFunction);
    }

    if ((_SWIFTAnalyzer == nullptr) && (_State._Transform == Transform::SWIFT))
    {
        _SWIFTAnalyzer = new SWIFTAnalyzer(&_State, _State._SampleRate, ChannelCount, ChannelSetup);

        _SWIFTAnalyzer->Initialize(_FrequencyBands);
    }
}

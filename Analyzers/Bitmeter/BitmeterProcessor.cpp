
/** $VER: BitmeterProcessor.cpp (2026.10.27) P. Stuer **/

#include <pch.h>

#include "BitmeterProcessor.h"

namespace BitMeter
{

/// <summary>
/// Configures this instance.
/// </summary>
void processor_t::Configure(const graph_options_t * options) noexcept
{
    __super::Configure(options);
}

/// <summary>
/// Resets this instance.
/// </summary>
void processor_t::Reset() noexcept
{
    __super::Reset();

    _BitActiveChannelMask = 0;

    Initialize((uint32_t) Channels::ConfigStereo);
}

/// <summary>
/// Process the chunk data for the bit meter.
/// </summary>
void processor_t::Process(const audio_chunk & chunk) noexcept
{
    if (chunk.is_empty())
        return;

    if ((_ChannelCount != chunk.get_channel_count()) || (_ChannelConfig != chunk.get_channel_config()))
        Reset();

    _ChannelCount  = chunk.get_channel_count();
    _ChannelConfig = chunk.get_channel_config();

    _ChannelMask   = _ChannelConfig & _Options->_ActiveChannelMask;

    if (_ChannelMask == 0)
        return; // None of the selected channels are present in this chunk.

    Initialize(_ChannelMask);

    const audio_sample * Frames = chunk.get_data();
    const size_t FrameCount = chunk.get_sample_count(); // get_sample_count() actually returns the number of frames.

    const auto MaxInteger = (audio_sample) (1LL << (_State._BitsPerInteger - 1));

    const audio_sample * CurrentFrame = Frames;

    for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
    {
        const audio_sample * CurrentSample = CurrentFrame;

        size_t i = 0;

        uint32_t AvailableChannelMask = chunk.get_channel_config();         // Mask containing the channels in the audio chunk.
        uint32_t ActiveChannelMask    = _Options->_ActiveChannelMask;  // Mask containing the channels selected by the user for processing.

        while ((AvailableChannelMask & ActiveChannelMask) != 0)
        {
            if (AvailableChannelMask & 1)
            {
                if ((ActiveChannelMask & 1) && (i < _BitMeasurements.size()))
                {
                    uint64_t SampleBits;

                    if (_State._BitMeterMode == BitMeterMode::FloatingPoint)
                    {
                        using sample_bits_t = std::conditional_t<sizeof(audio_sample) == 8, uint64_t, uint32_t>;

                        // Test pattern: 64-bit: 0b1101010101011001111111111111111111111111111111111111111111111001 / 32-bit: 0b11010101010000000000000000000001
                        SampleBits = std::bit_cast<sample_bits_t>(*CurrentSample);
                    }
                    else
                        SampleBits = (uint64_t) std::abs(*CurrentSample * MaxInteger);

                    for (auto & BitCount : _BitMeasurements[i].BitCounts)
                    {
                        if (SampleBits == 0)
                            break;

                        if (SampleBits & 1)
                            ++BitCount;

                        SampleBits >>= 1;
                    }

                    ++i;
                }

                ++CurrentSample;
            }

            AvailableChannelMask >>= 1;
            ActiveChannelMask    >>= 1;
        }

        CurrentFrame += _ChannelCount;
    }

    // Scale the bit counters to range [0, 1].
    for (auto & m : _BitMeasurements)
    {
        for (auto & BitCount : m.BitCounts)
            BitCount /= (double) FrameCount;

        // Reverse the array to get: Index 0 = Sign bit, Index 1 - 11 = Exponent bits, Index 12 - 63 = Mantissa bits
        std::reverse(m.BitCounts.begin(), m.BitCounts.end());
    }
}

/// <summary>
/// Initializes the bit measurements before processing an audio chunk.
/// </summary>
void processor_t::Initialize(uint32_t activeChannelMask) noexcept
{
    if (_BitActiveChannelMask != activeChannelMask)
    {
        // The chunk configuration has changed. Recreate the measurements, one per selected channel.
        static const WCHAR * ChannelNames[] =
        {
            L"FL", L"FR", L"FC",
            L"LFE",
            L"BL", L"BR", 
            L"FCL", L"FCR",
            L"BC", L"SL", L"SR", L"TC",
            L"TFL", L"TFC", L"TFR", L"TBL", L"TBC", L"TBR",
        };

        const size_t n = (size_t) ((_State._BitMeterMode == BitMeterMode::FloatingPoint) ? audio_sample_size : _State._BitsPerInteger);

        size_t i = 0;

        _BitMeasurements.clear();

        for (uint32_t SelectedChannels = activeChannelMask; (SelectedChannels != 0) && (i < _countof(ChannelNames)); SelectedChannels >>= 1, ++i)
        {
            if (SelectedChannels & 1)
                _BitMeasurements.push_back({ (Channels) (1 << i), ChannelNames[i], n });
        }

        _BitActiveChannelMask = activeChannelMask;
    }
    else
    {
        for (auto & m : _BitMeasurements)
            std::fill(m.BitCounts.begin(), m.BitCounts.end(), 0.);
    }
}

}

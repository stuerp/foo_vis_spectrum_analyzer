
/** $VER: BitmeterProcessor.h (2026.10.27) P. Stuer **/

#pragma once

#include "AudioProcessor.h"

namespace BitMeter
{

/// <summary>
/// Represents a bit meter measurement of one channel.
/// </summary>
struct bit_measurement_t : measurement_t
{
    bit_measurement_t(Channels channel, const WCHAR * channelName, size_t bitCount) noexcept : measurement_t(channel, channelName)
    {
        BitCounts.resize(bitCount, 0.);
    }

    std::vector<double> BitCounts; // Occurrance count of each bit
};

class processor_t final : public audio_processor_t
{
public:
    processor_t(state_t & state) : audio_processor_t(state) { }

    processor_t(const processor_t &) = delete;
    processor_t(processor_t &&) = delete;

    processor_t & operator=(const processor_t &) = delete;
    processor_t & operator=(processor_t &&) = delete;

    virtual ~processor_t() = default;

public:
    void Configure(const graph_options_t * options) noexcept override final;
    void Reset() noexcept override final;
    void Process(const audio_chunk & chunk) noexcept override final;

private:
    void Initialize(uint32_t channelMask) noexcept;

public:
    std::vector<bit_measurement_t> _BitMeasurements;
    uint32_t _BitActiveChannelMask;
};

}
